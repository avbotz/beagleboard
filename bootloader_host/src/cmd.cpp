/******************************************************************************\
* testing
 * 
 *  cmd will parse the Standard C++ 'ArgV' command array returning both options
 *  and arguments.  A cmd object is first initialized with 'ArgV' and a list 
 *  of valid option characters.  The method 'Next()' returns a character that
 *  is one of the following:
 *  
 *  '\0'  -  If we have hit the end of command array.
 *
 *  '.'   -  If we hit a command line argument, a pointer to the argument
 *           may be obtained by invoking the 'Arg()' method.
 *
 *  '-'   -  An option introduction character was found, but no option (some
 *           utilities -- like the unix 'tar' command  -- require this 
 *           functionality).  
 *
 *  other -  If none of the above characters are found, then one of the 
 *           characters from the option character list was found.  The
 *           character itself can be obtained using the 'Char()' method.
 *           If the option character was followed by a colon, then the
 *           option is expected to have an argument.  This argument may
 *           immediately follow the option character, as in '-I/usr', or
 *           may be separate from the option character as in '-I /usr'.
 *           This argument may be obtained using the 'Arg()' method.
 *           If no argument is supplied, then 'Arg()' will return NULL.
 *
 *      EXAMPLE:
 *
 *    cmd_cCmd Cmd(ArgV, "cd:");
 *
 *    while (Cmd.Next()) {
 *      switch (Cmd.Option()) {
 *        case 'c':
 *          cout << "-c flag" << endl;
 *          break;
 *        case 'd':
 *          if (Cmd.Arg() == NULL)
 *            cerr << "-d requires arg" << endl;
 *          else
 *            cout << "-d: Arg: " << Cmd.Arg() <<endl;
 *
 *          break;
 *
 *        case '.':
 *          cout << "Arg: " << Cmd.Arg() << endl;
 *          break;
 * 
 *        case '-':
 *          cout << "-" << endl;
 *          break;
 *
 *        default:
 *          cout << "Unknown option `" << Cmd.Char() << "`" << endl;
 *          break;
 *      }
 *    }
 *
\******************************************************************************/
#include "stdafx.h"


/******************************************************************************/
cmd_cCmd::cmd_cCmd( char * ppArgV[],  char * pOptionList)
: m_ppArgV(ppArgV), 
  m_ppCurrentArg(ppArgV+1),
  m_pOptionList(pOptionList)
{ }
/******************************************************************************/
char cmd_cCmd::Next()
{
	if (*m_ppCurrentArg == NULL)
	{
		return 0;
	}

	if ((*m_ppCurrentArg)[0] != '-')
	{
		/* Regular Argument */
		m_pArg = *m_ppCurrentArg++;
		return m_Option = '.';
	}

	if ((*m_ppCurrentArg)[1] == '\0') 
	{
		/* Lone '-' */
		m_ppCurrentArg++;
		return m_Option = '-';
	}

	const char * pOption = strchr(m_pOptionList, (*m_ppCurrentArg)[1]);

	if (pOption == NULL)
	{
		/* Unknown option char */
		return m_Option = (*m_ppCurrentArg)[1];
	}

	m_Option = *pOption;
    
	if (pOption[1] != ':')
	{
		/* Not expecting an option argument */
		m_ppCurrentArg++;
		return m_Option;  
	}

	if ((*m_ppCurrentArg)[2] != '\0')
	{
		/* arg follows option char */
		m_pArg = &(*m_ppCurrentArg)[2];
		m_ppCurrentArg++;
		return m_Option;  
	}

	m_ppCurrentArg++;

	if (*m_ppCurrentArg == NULL || (*m_ppCurrentArg)[0] == '-')
	{
		/* Option arg expected, none found */
		m_pArg   = NULL;
		return m_Option;
	}

	m_pArg = *m_ppCurrentArg++;

	return m_Option;
}
