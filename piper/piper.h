
#ifndef PIPER_H_
#	define PIPER_H_
#	include <cstdlib>  //  for size_t
#	include <cassert>
#	include <string>
#	include <list>

	using std::size_t;
	using std::string;


	class
Piper
{
  public:
  	     Piper();
	void Reset();
	void StoreChar(char c);
	int  GetNextError(string & strFileName, size_t *nLine, string & strMessage);
	
  private:
  	friend void PiperPrivatesTestor();
               void checkIfThisIsAnErrorString();

        string      m_strError;

	typedef std::list< string >
	            list_string_t;

	list_string_t
	            m_list_errors;

};

#endif // ! PIPER_H_
