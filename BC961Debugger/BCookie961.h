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

#endif
