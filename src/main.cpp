#include "Logger.hpp"
#include "enum_tools.hpp"
#include <iostream>



void test(){
    LOG_ENTRY_EXIT;
}
int main()
{
    LOG_ENTRY_EXIT
    test();
    LOG(LogLevel::Info) << "Hello World";
}
