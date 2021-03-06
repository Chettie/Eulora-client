/*
 * pawsactivemagicwindow.cpp
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <psconfig.h>

// COMMON INCLUDES
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "util/strutil.h"

// CLIENT INCLUDES
#include "../globals.h"

// PAWS INCLUDES
#include "pawsactivemagicwindow.h"
#include "paws/pawslistbox.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "pawsconfigpopup.h"

#define BUFF_CATEGORY_PREFIX    "+"
#define DEBUFF_CATEGORY_PREFIX  "-"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsActiveMagicWindow::pawsActiveMagicWindow() :
    buffList(NULL),
    lastIndex(0),
    configPopup(NULL),
    show(true),
    useImages(true),
    autoResize(true),
    showEffects(false)
{
    OnResize(); //get orientation set correctly
}

void pawsActiveMagicWindow::OnResize()
{
    if(GetScreenFrame().Width() > GetScreenFrame().Height())
    {
        Orientation = ScrollMenuOptionHORIZONTAL;
    }
    else
    {
        Orientation = ScrollMenuOptionVERTICAL;
    }
    if(buffList!=NULL)
    {
        buffList->SetOrientation(Orientation);
        buffList->OnResize();
    }

}

bool pawsActiveMagicWindow::PostSetup()
{
    pawsWidget::PostSetup();

    buffList  = (pawsScrollMenu*)FindWidget("BuffBar");
    if(!buffList)
        return false;
    buffList->SetEditLock(ScrollMenuOptionDISABLED);
    if(autoResize)
    {
        buffList->SetLeftScroll(ScrollMenuOptionDISABLED);
        buffList->SetRightScroll(ScrollMenuOptionDISABLED);
        buffList->AutoResize();
        AutoResize();
    }
    else
    {
        buffList->SetLeftScroll(ScrollMenuOptionDYNAMIC);
        buffList->SetRightScroll(ScrollMenuOptionDYNAMIC);
    }

    showWindow              = (pawsCheckBox*)FindWidget("ShowActiveMagicWindow");
    if(!showWindow)
        return false;

    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_ACTIVEMAGIC);

    // If no active magic, hide the window.
    if(buffList->GetSize() < 1 && showWindow->GetState() == false)
    {
        showWindow->Hide();
    }
    else
    {
        showWindow->Show();
    }

    if(!LoadSetting())
        return false;

    return true;
}

bool pawsActiveMagicWindow::Setup(iDocumentNode* node)
{
    useImages  = true;

    if(node->GetAttributeValue("name") && strcmp("ActiveMagicWindow", node->GetAttributeValue("name"))==0)
    {
        csRef<iDocumentAttributeIterator> attiter = node->GetAttributes();
        csRef<iDocumentAttribute> subnode;

        while(attiter->HasNext())
        {

            subnode = attiter->Next();
            if(strcmp("useImages", subnode->GetName())==0)
            {
                if(strcmp("false", subnode->GetValue())==0)
                {
                    useImages=false;
                }
            }
            else if(strcmp("autoResize", subnode->GetName())==0)
            {
                if(strcmp("false", subnode->GetValue())==0)
                {
                    autoResize=false;
                }
            }
            else if(strcmp("showEffects", subnode->GetName())==0)
            {
                if(strcmp("true", subnode->GetValue())==0)
                {
                    showEffects=true;
                }
            }
        }
    }

    return true;
}


void pawsActiveMagicWindow::HandleMessage(MsgEntry* me)
{
    if(!configPopup)
        configPopup = (pawsConfigPopup*)PawsManager::GetSingleton().FindWidget("ConfigPopup");

    psGUIActiveMagicMessage incoming(me);
    if( !incoming.valid )
        return;

    // Use signed comparison to handle sequence number wrap around.
    if( (int)incoming.index - (int)lastIndex < 0 )
    {
        return;
    }
    csList<csString> rowEntry;
    show = showWindow->GetState() ? false : true;
    if(!IsVisible() && psengine->loadstate == psEngine::LS_DONE && show)
        ShowBehind();

    size_t    numSpells=incoming.name.GetSize();

    buffList->Clear();

    if( numSpells==0 )
    {
        Hide();
    }
    else
    {
        for( int i=0; i<numSpells; i++ )
        {
            if(incoming.duration[i]==0 && showEffects==false)
            {
    	        continue;
            }
    	    rowEntry.PushBack(incoming.name[i]);
    
            if(useImages)
            {
                csRef<iPawsImage> image;
                if(incoming.image[i].Length() >0)
                {
                    image = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(incoming.image[i]);
                }
                if(image)
                {
                    buffList->LoadSingle(incoming.name[i], incoming.image[i], incoming.name[i], csString(""), 0, this, false);
                }
                else
                {
                    if( incoming.type[i]==BUFF )
                    {
                        buffList->LoadSingle(incoming.name[i], csString("/planeshift/materials/crystal_ball_icon.dds"), incoming.name[i], csString(""), 0, this, false);
                    }
                    else
                    {
                        buffList->LoadSingle(incoming.name[i], csString("danger_01"), incoming.name[i], csString(""), -1, this, false);
                    }
                }
            }
            else
            {
                buffList->LoadSingle(incoming.name[i], csString(""), incoming.name[i], csString(""), 0, this, false);
            }
        }
        if(autoResize)
        {
            AutoResize();
        }
        else
        {
            buffList->Resize();
        }
    }
}

void pawsActiveMagicWindow::AutoResize()
{
    int buffSize = 0,
        t = 0;

    if(!buffList)
    {
        return;
    }

    if(Orientation == ScrollMenuOptionHORIZONTAL)
    {
        buffSize = buffList->AutoResize();
        if(buffSize == 0)
        {
            buffSize = buffList->GetButtonWidth();
        }

        SetSize(buffSize+buffList->GetButtonWidth(), GetScreenFrame().Height());
        if(GetScreenFrame().xmax > psengine->GetG2D()->GetWidth())   //sticking off the edge of the screen
        {
            //t = GetScreenFrame().xmax - psengine->GetG2D()->GetWidth();
            t = buffList->GetTotalButtonWidth();
            if(GetScreenFrame().xmin - t < 0)
            {
                t = -GetScreenFrame().xmin; //should put the window at the left ede of the screen
            }
            else
            {
                t = -(GetScreenFrame().xmax - psengine->GetG2D()->GetWidth());
            }
            MoveDelta(t, 0);
        }
    }
    else
    {
        buffSize = buffList->AutoResize();
        if(buffSize == 0)
        {
            buffSize = buffList->GetButtonHeight();
        }

        SetSize(GetScreenFrame().Width(), buffSize+buffList->GetButtonHeight());

        if(GetScreenFrame().ymax > psengine->GetG2D()->GetHeight())   //sticking off the bottom of the screen
        {
            MoveDelta(0, -(GetScreenFrame().ymax - psengine->GetG2D()->GetHeight()));
        }
    }
}

void pawsActiveMagicWindow::Close()
{
    Hide();
}

bool pawsActiveMagicWindow::LoadSetting()
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root,activeMagicNode, activeMagicOptionsNode;
    csString option;

    doc = ParseFile(psengine->GetObjectRegistry(), CONFIG_ACTIVEMAGIC_FILE_NAME);
    if(doc == NULL)
    {
        //load the default configuration file in case the user one fails (missing or damaged)
        doc = ParseFile(psengine->GetObjectRegistry(), CONFIG_ACTIVEMAGIC_FILE_NAME_DEF);
        if(doc == NULL)
        {
            Error2("Failed to parse file %s", CONFIG_ACTIVEMAGIC_FILE_NAME_DEF);
            return false;
        }
    }

    root = doc->GetRoot();
    if(root == NULL)
    {
        Error1("activemagic_def.xml or activemagic.xml has no XML root");
        return false;
    }

    activeMagicNode = root->GetNode("activemagic");
    if(activeMagicNode == NULL)
    {
        Error1("activemagic_def.xml or activemagic.xml has no <activemagic> tag");
        return false;
    }

    // Load options for Active Magic Window
    activeMagicOptionsNode = activeMagicNode->GetNode("activemagicoptions");
    if(activeMagicOptionsNode != NULL)
    {
        csRef<iDocumentNodeIterator> oNodes = activeMagicOptionsNode->GetNodes();
        while(oNodes->HasNext())
        {
            csRef<iDocumentNode> option = oNodes->Next();
            csString nodeName(option->GetValue());

            if(nodeName == "showWindow")
                showWindow->SetState(!option->GetAttributeValueAsBool("value"));
        }
    }

    return true;
}

void pawsActiveMagicWindow::SaveSetting()
{
    csRef<iFile> file;
    file = psengine->GetVFS()->Open(CONFIG_ACTIVEMAGIC_FILE_NAME,VFS_FILE_WRITE);

    csRef<iDocumentSystem> docsys;
    docsys.AttachNew(new csTinyDocumentSystem());

    csRef<iDocument> doc = docsys->CreateDocument();
    csRef<iDocumentNode> root,defaultRoot, activeMagicNode, activeMagicOptionsNode, showWindowNode;

    root = doc->CreateRoot();

    activeMagicNode = root->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    activeMagicNode->SetValue("activemagic");

    activeMagicOptionsNode = activeMagicNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    activeMagicOptionsNode->SetValue("activemagicoptions");

    doc->Write(file);
}
