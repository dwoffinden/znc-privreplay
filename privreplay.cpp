#include "znc/Modules.h"
#include "znc/User.h"
#include <sys/stat.h>

using std::vector;

class CPrivReplay : public CModule
{
public:
    MODCONSTRUCTOR(CPrivReplay)
    {
        AddCommand(CString("clear"), static_cast<CModCommand::ModCmdFunc>(&CPrivReplay::ClearPrivateMessages));
    }

    virtual ~CPrivReplay()
    {
        RemCommand(CString("clear"));
    }

    void ClearPrivateMessages(const CString &command)
    {
        if (command != "clear") {
            return;
        }

        m_vMessages.clear();

        PutModule(CString("Private messages cleared"));
    }

    virtual EModRet OnPrivMsg(CNick& Nick, CString& sMessage)
    {
        CString msg = m_pUser->AddTimestamp(CString("[->] ") + sMessage);
        StoreMessage(Nick, msg);

        return (CONTINUE);
    }

    virtual EModRet OnUserRaw(CString &sLine)
    {
        //PutModule("Raw: " + sLine);
        if (sLine.Left(7).Equals("PRIVMSG"))
        {
            //PutModule("OUTBOUND PRIVMSG");
            VCString vsRet;
            sLine.Split(" ", vsRet);

            // Check the prefix on the to name and ignore special users, control channels and channels.
            CString toNamePrefix = vsRet[1].Left(1);
            if (toNamePrefix.Equals("*") || toNamePrefix.Equals("&") || toNamePrefix.Equals("#"))
            {
                return CONTINUE;
            }

            CString msg = vsRet[2];
            msg.LeftChomp(1);
            
            // Don't process outgoing CTCP
            if (msg.Left(1).Equals("\x01"))
              return (CONTINUE);
            
            for (unsigned int c = 3; vsRet.size() > c; c++)
            {
                msg += " " + vsRet[c];
            }

            CString timestamp_msg = m_pUser->AddTimestamp(CString("[<-] ") + msg);
            StoreRawMessage(":" + vsRet[1] + " PRIVMSG " + m_pUser->GetNick() + " :" + timestamp_msg);
        }
        return (CONTINUE);
    }

    virtual void OnClientLogin()
    {
        ReplayMessages();
    }

private:
    void StoreMessage(const CNick & Nick, CString & sMessage)
    {
        StoreRawMessage(":" + Nick.GetNickMask() + " PRIVMSG " + m_pUser->GetNick() + " :" + sMessage);
    }

    void StoreRawMessage(const CString & sText)
    {
        m_vMessages.push_back(sText);
    }

    void ReplayMessages()
    {
        if (!m_vMessages.empty())
        {
            vector<CString>::iterator iter;
            for (iter = m_vMessages.begin(); iter != m_vMessages.end(); iter++)
            {
                PutUser(*iter);
            }
        }
    }

    vector<CString> m_vMessages;
};

MODULEDEFS(CPrivReplay, "Stores private messages and replays them");

