
  /*  piper.cpp

      Test-infected module that filters a character stream looking 
      for error messages, then stashes the first one

  */

#include "piper.h"
#include <cstring>
#include <cassert>
#include <kregexp.h>


Piper::Piper()
	{
	Reset();
	}


        void
Piper::Reset()
	{
        m_strError = "";
        m_list_errors = list_string_t();
	}


	static bool
isColonNumberColon(char const *z)  /* match :9: */
{

    static KRegExp regExp( "\\:([0-9]+)\\:" );
    regExp.match(z);
    char const * tag (regExp.group(0));
    return tag && strlen(tag) > 0;

}


        void
Piper::checkIfThisIsAnErrorString()
{

	assert (isColonNumberColon(":10:"));
	assert (!isColonNumberColon(":10"));
	assert (!isColonNumberColon("10:"));
	assert (!isColonNumberColon("10"));
	assert (!isColonNumberColon(":"));

        char const *z = m_strError.c_str();
	static char const strEXTRA [] = "In file included from ";
	
	if (strncmp (m_strError.c_str(), strEXTRA, sizeof strEXTRA - 1))
		{
	        for (;  *z;  ++z)
 			{
			if (isColonNumberColon(z))
	               	{
 	                using std::make_pair;
 	                m_list_errors.push_back (m_strError);
  	                break;
                        }
    			}
		}
	m_strError = "";

}


        void
Piper::StoreChar(char ch)
{

        if (ch == '\n')
                checkIfThisIsAnErrorString();
        else
                {
                m_strError += ch;
                }

}


        int
Piper::GetNextError(string & strFileName, size_t *nLine, string & strMessage)
{

	list_string_t::size_type amount (m_list_errors.size());

        if (amount)
                {
                list_string_t::iterator it (m_list_errors.begin());
                string strError (*it);                                
                m_list_errors.erase(it);
                
                assert (amount == m_list_errors.size() + 1);
                
  //  TODO: Replace following muck with more KRegExp...                
                
                char const *z = strError.c_str();
                size_t extent = 0;
		strFileName = "";

                while (*z && *z != ':')
                	strFileName += *z++, ++extent;

                strFileName += '\0';
                char *nextZ (NULL);
                *nLine = *z?  (size_t) (strtol(z + 1, &nextZ, 10)):  0u;
                z = nextZ;
                
                if (z)
                	{
                	assert (*z == ':' || *z == '\0');
                	extent = 0;
                	strMessage = "";

                	if (*z)
	                	while (*++z)  strMessage += *z;

	            strMessage += '\0';
                	}
                return 1;
                }
        else
                return 0;

}

