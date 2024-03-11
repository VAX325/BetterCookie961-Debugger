#pragma once
#ifndef BCOOKIE961_H
#define BCOOKIE961_H

#include <atomic>
#include <vector>

int bc961_main(std::atomic_bool* run_ptr);

const std::vector<int>& getArray();
const int getPointerLocation();

#endif
