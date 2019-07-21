/* 
* f_ui_concept.cpp
*
* Created: 16.07.2018 20:11:40
* Author: F-NET-ADMIN
*/


#include "f_ui_concept.h"

char c_main_menu[] =
R"xx(SETTINGS
SHUTDOWN
PUMP
)xx";

fsl::ui::char_to_bool_function x = [](char x){ return x == '0'; };

//fsl::ui::item_selector<> selector();

