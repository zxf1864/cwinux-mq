#ifndef __ZK_COMMON_H__
#define __ZK_COMMON_H__

#include "ZkToolAdaptor.h"
#include <getopt.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

void output(FILE* fd, int result, int zkstate, char const* format, char const* msg);

bool readFile(string const& strFileName, string& strData);

class ZkGetOpt
{
public:
	enum {
		/// Doesn't take an argument.
		NO_ARG = 0,
		/// Requires an argument, same as passing ":" after a short option
		/// character in optstring.
		ARG_REQUIRED = 1,
		/// Argument is optional, same as passing "::" after a short
		/// option character in optstring.
		ARG_OPTIONAL = 2
	};
	///���캯��
	ZkGetOpt (int argc,
		char **argv,
		char const* optstring = "");
	///��������
	~ZkGetOpt (void);
public:
	///����long option��Ӧ��short option
	int long_option (char const *name, int short_option, int has_arg = NO_ARG);
	/**
	@brief ��ȡ��һ������
	@return ��getopt_long()��ͬ
	*/
	int next();
	///��ȡoption�Ĳ���
	char *opt_arg (void) const;
	///���ص�ǰ��option
	int opt_opt (void) const;
	///���ص�ǰ������index
	int opt_ind (void) const;
	///���ص�ǰ��long option��
	char const* long_option() const;
private:
	int             m_argc;
	char **         m_argv;
	char const*     m_optString;
	struct option*   m_longOptions;
	int             m_longindex;
	int             m_longOptionsNum;
	int             m_longOptionsSize;
	int             m_opt;
};

#endif 
