
#include "piper.h"
#include <iostream>

	using std::cout;
	using std::endl;


	static int
emit (char const * z)
	{
	cout << z << endl;
	}

#define DEBUG_(x) cout << #x ": " << x << endl
	
	
	void
PiperPrivatesTestor()
{
	Piper aPiper;
	
	aPiper.m_strError = "no error";
	aPiper.checkIfThisIsAnErrorString();
	assert (aPiper.m_list_errors.size() == 0);
	assert (aPiper.m_strError.size() == 0);

	aPiper.m_strError = "no error:";
	aPiper.checkIfThisIsAnErrorString();
	assert (aPiper.m_list_errors.size() == 0);
	assert (aPiper.m_strError.size() == 0);

	aPiper.m_strError = "no error:9";
	aPiper.checkIfThisIsAnErrorString();
	assert (aPiper.m_list_errors.size() == 0);
	assert (aPiper.m_strError.size() == 0);

	aPiper.m_strError = "fileName:9:message";
	aPiper.checkIfThisIsAnErrorString();
	assert (aPiper.m_list_errors.size() == 1);

}

	
        static void
PiperTestor()
{

	PiperPrivatesTestor();

#define FILENAME_ "in/a/metal/mood.h"
#define ERROR_MESSAGE_ " evil syntax error"
#define ERROR_MESSAGE_2_ " the usual helplessly cascading errors"

        static char const strResults [] =
                "Whizzo Compilers & Body Shop, (c) 1999\n"
                "ringer: this is not an error line\n"
                "ringer::this is not an error line either\n"
                "ringer:5 neither is this\n"
"In file included from /export/home/phlip/work/projects/tilers/OExtractedData_.cpp:12:\n"
                FILENAME_":10:" ERROR_MESSAGE_ "\n"
                FILENAME_":12:" ERROR_MESSAGE_2_ "\n"
                "suffragette/city.h:13\n";

	char *q (NULL);
	long gotTen = strtol ("10:yo", &q, 10);
	assert (gotTen == 10);
	assert (q);
	assert (*q == ':');

        char const *z = strResults;
        int got;

        string strFileName;
        Piper aPiper;
        aPiper.Reset();
        while(*z)  aPiper.StoreChar(*z++);
        size_t nLine = 0;
	string strMessage;
        got = aPiper.GetNextError(strFileName, &nLine, strMessage);
        assert(got);
        
        //DEBUG_('"' << strFileName << '"');
        //DEBUG_('"' << FILENAME_ << '"');
        
        // string == string is broke or something...
        
        assert(0 == strcmp(strFileName.c_str(), FILENAME_));
        assert(0 == strcmp(strMessage.c_str(), ERROR_MESSAGE_));
        assert(nLine == 10);

  //  Try to get the next error

        got = aPiper.GetNextError(strFileName, &nLine, strMessage);
        assert(got);
        
        assert(0 == strcmp(strFileName.c_str(), FILENAME_));
        assert(0 == strcmp(strMessage.c_str(), ERROR_MESSAGE_2_));
        assert(nLine == 12);

  //  No more errors to pop...
       
        got = aPiper.GetNextError(strFileName, &nLine, strMessage);
        assert(!got);

        aPiper.Reset();
        z = "No Errors Here";
        while(*z)  aPiper.StoreChar(*z++);
        got = aPiper.GetNextError(strFileName, &nLine, strMessage);
        assert(!got);
        emit("All tests passed.");

}


	int
main()
{

	PiperPrivatesTestor();
	PiperTestor();

}
