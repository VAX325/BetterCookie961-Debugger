#pragma once
#ifndef BCOOKIE961_H
#define BCOOKIE961_H

#include <atomic>
#include <vector>

class CExitException : public std::exception
{
public:
	CExitException() = default;
	virtual ~CExitException() = default;
};

int bc961_main_file(std::atomic_bool* run_ptr, const std::string_view file);
int bc961_main_shell(std::atomic_bool* run_ptr);

const std::vector<int>& getArray();
const int getPointerLocation();

void setBreakpoint(std::string_view::size_type pos, bool enable);

const char DebuggerVersion[8 + 1] = {
	// First month letter, Oct Nov Dec = '1' otherwise '0'
	(__DATE__[0] == 'O' || __DATE__[0] == 'N' || __DATE__[0] == 'D') ? '1' : '0',

	// Second month letter
	(__DATE__[0] == 'J')   ? ((__DATE__[1] == 'a') ? '1' : // Jan, Jun or Jul
                                ((__DATE__[2] == 'n') ? '6' : '7'))
	: (__DATE__[0] == 'F') ? '2'
						   : // Feb
		(__DATE__[0] == 'M') ? (__DATE__[2] == 'r') ? '3' : '5'
							 : // Mar or May
		(__DATE__[0] == 'A') ? (__DATE__[1] == 'p') ? '4' : '8'
							 : // Apr or Aug
		(__DATE__[0] == 'S') ? '9'
							 : // Sep
		(__DATE__[0] == 'O') ? '0'
							 : // Oct
		(__DATE__[0] == 'N') ? '1'
							 : // Nov
		(__DATE__[0] == 'D') ? '2'
							 : // Dec
		0,

	'.',

	// First day letter, replace space with digit
	__DATE__[4] == ' ' ? '0' : __DATE__[4],

	// Second day letter
	__DATE__[5],

	'.',

	// YY year
	__DATE__[9], __DATE__[10],

	'\0'
};

#endif
