#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <atomic>
#include <mutex>
#include <stack>
#include <unordered_set>
#include <stdlib.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <windows.h>
#include <regex>

#include "BCookie961.h"

/*


UPDATE LOG:

3.2 - Fixed Random Number Generator

3.3 - Comments added!

3.4 - Fixed 6-1 cycles and if constructions!

3.5 - Added P and J commands!

3.6 - Added _(break), x/m[+/-0/1], and G

3.7 - Made cross-platform and fixed bugs (comments are now working just fine!)


*/

std::atomic_bool debuggerStep = false;
std::atomic_bool debuggerWait = false;
std::string_view debuggerCurrentCode = "";
std::string_view::size_type debuggerCurrentCodePos = 0;

std::mutex mutex;
std::unordered_set<std::string_view::size_type> debuggerBreakpoints;

std::atomic_bool* runPtr;

std::vector<int> array{0};
int pointerLocation = 0;

const std::vector<int>& getArray() { return array; }

const int getPointerLocation() { return pointerLocation; }

void setBreakpoint(std::string_view::size_type pos, bool enable)
{
	std::lock_guard<std::mutex> lock(mutex);
	if (enable)
		debuggerBreakpoints.insert(pos);
	else
		debuggerBreakpoints.erase(pos);
}

bool endsWith(const std::string_view str, const std::string_view suffix)
{
	if (str.length() < suffix.length()) { return false; }
	return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

//Delete Comments from code
static std::string preprocessCode(const std::string& code)
{
	std::string result = code;
	std::regex singleLineComment("//.*");
	std::regex multiLineComment("/\\*[^*]*\\*+(?:[^/*][^*]*\\*+)*/");
	result = std::regex_replace(result, singleLineComment, "");
	result = std::regex_replace(result, multiLineComment, "");
	return result;
}

std::string readFile(const std::string_view fileName)
{
	std::ifstream f(fileName.data());
	if (!f.is_open())
	{
		std::cout << "Error! File not found!" << std::endl;
		exit(0);
	}
	f.seekg(0, std::ios::end);
	size_t size = f.tellg();
	std::string s;
	s.resize(size);
	f.seekg(0);
	f.read(&s[0], size);
	return s;
}

void ifFunc(std::string code);
void arythm(std::string code);

void interpret(std::string code)
{
	//const std::string code = preprocessCode(rawCode);
	int i = 0;
	int c = 0;
	while (i < code.length() && *runPtr)
	{
		/*std::lock_guard<std::mutex> lock(mutex);
		if (debuggerBreakpoints.contains(i))
		{
			debuggerWait = true;
		}*/

		if (debuggerWait)
		{
			debuggerCurrentCode = code;
			debuggerCurrentCodePos = i;
		}

		while (debuggerWait && *runPtr)
		{
			Sleep(25);
			if (debuggerStep) debuggerWait = false;
		}

		if (debuggerStep)
		{
			debuggerStep = false;
			debuggerWait = true;
		}

		if (code[i] == 'i')
		{
			if (pointerLocation > 0) { pointerLocation -= 1; }
		}
		else if (code[i] == 'I')
		{
			if (pointerLocation > 0)
			{
				pointerLocation -= 1;
				array[pointerLocation] = array[pointerLocation + 1];
			}
		}
		else if (code[i] == 'k')
		{
			pointerLocation += 1;
			if (array.size() <= pointerLocation) { array.push_back(0); }
		}
		else if (code[i] == 'K')
		{
			pointerLocation += 1;
			if (array.size() <= pointerLocation) { array.push_back(0); }
			array[pointerLocation] = array[pointerLocation - 1];
		}
		else if (code[i] == 'r')
		{
			std::string flnm;
			std::cout << "File Name: ";
			std::cin >> flnm;
			if (endsWith(flnm, ".bc961")) { interpret(readFile(flnm)); }
			else
			{
				std::cout << "Error! Unknown extension.";
				exit(0);
			}
		}
		else if (code[i] == 'N') { pointerLocation = 0; }
		else if (code[i] == 'P')
		{
			int tmp = pointerLocation;
			pointerLocation = array[pointerLocation];
			if (pointerLocation >= array.size()) { array.resize(pointerLocation + 1, 0); }
			array[pointerLocation] = tmp;
		}
		else if (code[i] == 'J')
		{
			pointerLocation = array[pointerLocation];
			if (pointerLocation >= array.size()) { array.resize(pointerLocation + 1, 0); }
		}
		else if (code[i] == 'n') { std::cout << array[pointerLocation]; }
		else if (code[i] == 'g') { std::cout << pointerLocation; }
		else if (code[i] == 'R') { array[pointerLocation] = rand() % 101; }
		else if (code[i] == 'c') { array[pointerLocation] += 1; }
		else if (code[i] == 'C') { array[pointerLocation] += 10; }
		else if (code[i] == 'o')
		{
			if (array[pointerLocation] > 0) { array[pointerLocation] -= 1; }
		}
		else if (code[i] == '-')
		{
			throw CExitException();
		}
		else if (code[i] == '>') { std::cout << std::endl; }
		else if (code[i] == 'O')
		{
			if (array[pointerLocation] > 10) { array[pointerLocation] -= 10; }
		}
		else if (code[i] == 'L') { array[pointerLocation] = 0; }
		else if (code[i] == '(')
		{
			int open_braces777 = 1;
			int ifend = i + 1;
			std::string to_execute = "";
			while (open_braces777 != 0)
			{
				if (code[ifend] == '(') { open_braces777++; }
				else if (code[ifend] == ')') { open_braces777--; }
				if (open_braces777 != 0) { to_execute += code[ifend]; }
				ifend++;
			}
			i = ifend - 1;
			ifFunc(to_execute);
		}
		/*else if (code[i] == '(') {
			int ifend = i + 1;
			std::string to_execute = "";
			while (code[ifend] != ')') {
				to_execute += code[ifend];
				ifend++;
				i++;
			}
			ifFunc(to_execute);
		}*/
		else if (code[i] == '{')
		{
			int arend = i + 1;
			std::string to_count = "";
			while (code[arend] != '}')
			{
				to_count += code[arend];
				arend++;
				i++;
			}
			arythm(to_count);
		}
		else if (code[i] == 'B')
		{
			if (code[++i] == '[')
			{
				int nigg = i + 1;
				std::string filename = "";
				while (code[nigg] != ']')
				{
					filename += code[nigg];
					nigg++;
					i++;
				}
				interpret(readFile(filename));
			}
			else
			{
				i--;
				std::cout << "Error! File not found!" << std::endl;
			}
		}
		else if (code[i] == 'S')
		{
			if (code[i + 1] == '[')
			{
				int nigge = i + 2;
				std::string times = "";
				while (code[nigge] != ']')
				{
					times += code[nigge];
					nigge++;
					i++;
				}
				Sleep(stoi(times));
			}
		}
		else if (code[i] == '9')
		{
			if (array[pointerLocation] >= 0) { std::cout << char(array[pointerLocation]); }
			else { std::cout << char(0); }
		}
		else if (code[i] == 'a')
		{
			if (array[pointerLocation] >= 0)
			{
				std::cout << array[pointerLocation] << " " << char(array[pointerLocation]) << std::endl;
			}
			else { std::cout << array[pointerLocation] << " " << char(0) << std::endl; }
		}
		else if (code[i] == 'e')
		{
			std::string x;
			std::cin >> x;
			int y;
			try
			{
				y = std::stoi(x);
			}
			catch (std::invalid_argument)
			{
				y = int(x[0]);
			}
			array[pointerLocation] = y;
		}
		else if (code[i] == '6')
		{
			if (array[pointerLocation] == 0)
			{
				int open_braces6 = 1;
				while (open_braces6 > 0)
				{
					i += 1;
					if (code[i] == '6') { open_braces6 += 1; }
					else if (code[i] == '1') { open_braces6 -= 1; }
					else if (code[i] == '[')
					{
						int open_brackets1 = 1;
						while (open_brackets1 > 0)
						{
							i += 1;
							if (code[i] == '[') { open_brackets1 += 1; }
							else if (code[i] == ']') { open_brackets1 -= 1; }
						}
						i += 1;
					}
				}
				i += 1;
			}
		}
		/*else if (code[i] == '6') {
			if (array[pointerLocation] == 0) {
				int open_braces = 1;
				while (open_braces > 0) {
					i += 1;
					if (code[i] == '6') {
						open_braces += 1;
					}
					else if (code[i] == '1') {
						open_braces -= 1;
					}
				}
			}
		}*/
		else if (code[i] == '1')
		{
			int open_braces = 1;
			while (open_braces > 0)
			{
				i -= 1;
				if (code[i] == '6') { open_braces -= 1; }
				else if (code[i] == '1') { open_braces += 1; }
				else if (code[i] == ']')
				{
					int open_brackets1 = 1;
					while (open_brackets1 > 0)
					{
						i -= 1;
						if (code[i] == '[') { open_brackets1 -= 1; }
						else if (code[i] == ']') { open_brackets1 += 1; }
					}
					i -= 1;
				}
			}
			i -= 1;
		}
		/*else if (code[i] == '1') {
			int open_braces = 1;
			while (open_braces > 0) {
				i -= 1;
				if (code[i] == '6') {
					open_braces -= 1;
				}
				else if (code[i] == '1') {
					open_braces += 1;
				}
			}
			i -= 1;
		}*/
		else if (code[i] == 'M')
		{
			if (code[i + 1] == '[')
			{
				int m_start = i + 2;
				int m_end = m_start;
				while (code[m_end] != ']')
				{
					m_end++;
				}
				std::string m_str = code.substr(m_start, m_end - m_start);
				int m = std::stoi(m_str);
				if (pointerLocation + m < 0)
				{
					std::cout << "Error! Pointer Location is negative." << std::endl;
					return;
				}
				else if (pointerLocation + m >= array.size())
				{
					while (array.size() <= pointerLocation + m)
					{
						array.push_back(0);
					}
				}
				int temp = array[pointerLocation];
				array[pointerLocation] = array[pointerLocation + m];
				array[pointerLocation + m] = temp;
				i = m_end;
				pointerLocation += m;
			}
			else
			{
				std::cout << "Error! Invalid syntax for M command." << std::endl;
				return;
			}
		}
		else if (code[i] == 'X')
		{
			if (code[i + 1] == '[')
			{
				int m_start = i + 2;
				int m_end = m_start;
				while (code[m_end] != ']')
				{
					m_end++;
				}
				std::string m_str = code.substr(m_start, m_end - m_start);
				int m = std::stoi(m_str);
				if (pointerLocation + m < 0)
				{
					std::cout << "Error! Pointer Location is negative." << std::endl;
					return;
				}
				else if (pointerLocation + m >= array.size())
				{
					while (array.size() <= pointerLocation + m)
					{
						array.push_back(0);
					}
				}
				array[pointerLocation + m] = array[pointerLocation];
				i = m_end;
				pointerLocation += m;
			}
			else
			{
				std::cout << "Error! Invalid syntax for M command." << std::endl;
				return;
			}
		}
		else if (code[i] == '/' && (i + 1 < code.length()) && code[i + 1] == '*')
		{
			i += 2;
			while (i < code.length() - 1 && !(code[i] == '*' && code[i + 1] == '/'))
			{
				i++;
			}
			i--;
			if (i < code.length() - 1) { i += 2; }
		}
		else if (code[i] == '_') { return; }
		else if (code[i] == 'x' && i + 4 < code.length() && code[i + 1] == '[' &&
				 (code[i + 2] == '+' || code[i + 2] == '-') && (code[i + 3] == '0' || code[i + 3] == '1') &&
				 code[i + 4] == ']')
		{
			int target_index = 0;
			if (code[i + 2] == '+') { target_index = array[pointerLocation + 1]; }
			else if (code[i + 2] == '-')
			{
				if (pointerLocation == 0)
				{
					std::cout << "Error! No left cell available." << std::endl;
					return;
				}
				target_index = array[pointerLocation - 1];
			}

			if (target_index < 0)
			{
				std::cout << "Error! Invalid target index." << std::endl;
				return;
			}

			if (array.size() <= target_index) { array.resize(target_index + 1, 0); }

			array[target_index] = array[pointerLocation];

			if (code[i + 3] == '1') { pointerLocation = target_index; }

			i += 4;
		}
		else if (code[i] == 'm' && i + 4 < code.length() && code[i + 1] == '[' &&
				 (code[i + 2] == '+' || code[i + 2] == '-') && (code[i + 3] == '0' || code[i + 3] == '1') &&
				 code[i + 4] == ']')
		{
			int target_index = 0;
			if (code[i + 2] == '+') { target_index = array[pointerLocation + 1]; }
			else if (code[i + 2] == '-')
			{
				if (pointerLocation == 0)
				{
					std::cout << "Error! No left cell available." << std::endl;
					return;
				}
				target_index = array[pointerLocation - 1];
			}

			if (target_index < 0)
			{
				std::cout << "Error! Invalid target index." << std::endl;
				return;
			}

			if (array.size() <= target_index) { array.resize(target_index + 1, 0); }

			array[target_index] = array[pointerLocation];
			array[pointerLocation] = 0;

			if (code[i + 3] == '1') { pointerLocation = target_index; }

			i += 4;
		}
		else if (code[i] == 'G') { array[pointerLocation] = pointerLocation; }
		i += 1;
	}

	debuggerCurrentCode = "";
	debuggerCurrentCodePos = 0;
}

void arythm(std::string code)
{
	int nums[1024];
	std::vector<int> actions;
	int pointnow = 0;
	bool actyes = false;

	for (int i = 0; i < code.length(); i++)
	{
		if (code[i] == 'k')
		{
			nums[pointnow] = array[pointerLocation + 1];
			pointnow += 1;
		}
		else if (code[i] == 'i')
		{
			nums[pointnow] = array[pointerLocation - 1];
			pointnow += 1;
		}
		else if (code[i] == 't')
		{
			nums[pointnow] = array[pointerLocation];
			pointnow += 1;
		}
		else if (code[i] == '+')
		{
			actions.push_back(1);
			actyes = true;
		}
		else if (code[i] == '-')
		{
			actions.push_back(0);
			actyes = true;
		}
		else if (code[i] == '*')
		{
			actions.push_back(2);
			actyes = true;
		}
		else if (code[i] == '%')
		{
			actions.push_back(3);
			actyes = true;
		}
	}

	if (actions[0] == 0) { array[pointerLocation] = nums[0] - nums[1]; }
	else if (actions[0] == 1) { array[pointerLocation] = nums[0] + nums[1]; }
	else if (actions[0] == 2) { array[pointerLocation] = nums[0] * nums[1]; }
	else if (actions[0] == 3) { array[pointerLocation] = nums[0] / nums[1]; }
	else
	{
		std::cout << "ERROR! Action not found!";
		exit(0);
	}
}

void ifFunc(std::string code)
{
	int nums[2];
	int action = 0;
	int pointnow = 0;
	bool ends = false;
	std::string execute = "";
	for (int i = 0; i < code.length(); i++)
	{
		if (code[i] == 'i')
		{
			nums[pointnow] = array[pointerLocation - 1];
			pointnow += 1;
		}
		else if (code[i] == 'k')
		{
			nums[pointnow] = array[pointerLocation + 1];
			pointnow += 1;
		}
		else if (code[i] == 't')
		{
			nums[pointnow] = array[pointerLocation];
			pointnow += 1;
		}
		else if (code[i] == '>') { action = 1; }
		else if (code[i] == '<') { action = 2; }
		else if (code[i] == '=') { action = 0; }
		else if (code[i] == '~') { action = 3; }
		else if (code[i] == '!')
		{
			execute = code.substr(i + 1, code.length());
			bool anif1 = false;
			bool anif2 = false;
			for (int p = 0; p < execute.length(); p++)
			{
				if (execute[p] == '(')
				{
					anif1 = true;
					for (int j = 0; j < execute.length(); j++)
					{
						if (execute[j] == ')') { anif2 = true; }
					}
				}
			}
			if (anif1 == true && anif2 == false) { execute += ')'; }
			ends = true;
			break;
		}
	}
	if (ends == true)
	{
		if (action == 0)
		{
			if (nums[0] == nums[1]) { interpret(execute); }
		}
		else if (action == 1)
		{
			if (nums[0] > nums[1]) { interpret(execute); }
		}
		else if (action == 2)
		{
			if (nums[0] < nums[1]) { interpret(execute); }
		}
		else if (action == 3)
		{
			if (nums[0] != nums[1]) { interpret(execute); }
		}
	}
	else { std::cout << "Error! If without body!" << std::endl; }
}

void ExceptionHandler(unsigned int, struct _EXCEPTION_POINTERS* ep)
{
	std::string reasonStr;
#define EXCEPTION_NAME(x)                                                                                              \
	case x:                                                                                                            \
		reasonStr = #x;                                                                                                \
		break

	switch (ep->ExceptionRecord->ExceptionCode)
	{
		EXCEPTION_NAME(EXCEPTION_ACCESS_VIOLATION);
		EXCEPTION_NAME(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
		EXCEPTION_NAME(EXCEPTION_BREAKPOINT);
		EXCEPTION_NAME(EXCEPTION_DATATYPE_MISALIGNMENT);
		EXCEPTION_NAME(EXCEPTION_FLT_DENORMAL_OPERAND);
		EXCEPTION_NAME(EXCEPTION_FLT_DIVIDE_BY_ZERO);
		EXCEPTION_NAME(EXCEPTION_FLT_INEXACT_RESULT);
		EXCEPTION_NAME(EXCEPTION_FLT_INVALID_OPERATION);
		EXCEPTION_NAME(EXCEPTION_FLT_OVERFLOW);
		EXCEPTION_NAME(EXCEPTION_FLT_STACK_CHECK);
		EXCEPTION_NAME(EXCEPTION_FLT_UNDERFLOW);
		EXCEPTION_NAME(EXCEPTION_ILLEGAL_INSTRUCTION);
		EXCEPTION_NAME(EXCEPTION_IN_PAGE_ERROR);
		EXCEPTION_NAME(EXCEPTION_INT_DIVIDE_BY_ZERO);
		EXCEPTION_NAME(EXCEPTION_INT_OVERFLOW);
		EXCEPTION_NAME(EXCEPTION_INVALID_DISPOSITION);
		EXCEPTION_NAME(EXCEPTION_NONCONTINUABLE_EXCEPTION);
		EXCEPTION_NAME(EXCEPTION_PRIV_INSTRUCTION);
		EXCEPTION_NAME(EXCEPTION_SINGLE_STEP);
		EXCEPTION_NAME(EXCEPTION_STACK_OVERFLOW);
		EXCEPTION_NAME(CONTROL_C_EXIT);
		default:
			reasonStr += "UNKNOWN!";
			break;
	}
#undef EXCEPTION_NAME

	throw std::runtime_error(reasonStr);
}

const std::string label = "Welcome to Better Cookie961 language Compiler v3.7";

int bc961_main_file(std::atomic_bool* run_ptr, const std::string_view filename)
{
	debuggerStep = false;
	debuggerWait = false;

	array = {0};
	pointerLocation = 0;

	runPtr = run_ptr;

	_set_se_translator(ExceptionHandler);

	srand(time(NULL));

	std::cout << label << std::endl;
	std::cout << " " << std::endl;

	if (endsWith(filename, ".bc961"))
	{
		const std::string foilData = readFile(filename);
		interpret(foilData);
		std::cout << " " << std::endl;
	}
	else
	{
		std::cout << "Error! Unknown extension.";
		return 0;
	}

	return 0;
}

int bc961_main_shell(std::atomic_bool* run_ptr)
{
	debuggerStep = false;
	debuggerWait = false;

	array = {0};
	pointerLocation = 0;

	runPtr = run_ptr;

	_set_se_translator(ExceptionHandler);

	srand(time(NULL));

	std::cout << label << std::endl;
	std::cout << " " << std::endl;
	int nig = 0;
	while (nig != 1)
	{
		std::string code;
		std::cout << "Code: ";
		std::getline(std::cin, code);
		interpret(code);
		std::cout << " " << std::endl;
		nig += 1;
	}

	return 0;
}
