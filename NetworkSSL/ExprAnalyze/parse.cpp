﻿/*
 **************************************************************
 *         C++ Mathematical Expression Toolkit Library        *
 *                                                            *
 * Simple Example 07                                          *
 * Author: Arash Partow (1999-2024)                           *
 * URL: https://www.partow.net/programming/exprtk/index.html  *
 *                                                            *
 * Copyright notice:                                          *
 * Free use of the Mathematical Expression Toolkit Library is *
 * permitted under the guidelines and in accordance with the  *
 * most current version of the MIT License.                   *
 * https://www.opensource.org/licenses/MIT                    *
 * SPDX-License-Identifier: MIT                               *
 *                                                            *
 **************************************************************
*/


#include <cstdio>
#include <string>

#include "exprtk.hpp"


template <typename T>
void logic()
{
    typedef exprtk::symbol_table<T> symbol_table_t;
    typedef exprtk::expression<T>   expression_t;
    typedef exprtk::parser<T>       parser_t;

    const std::string expression_string = "1 of test* or C";

    symbol_table_t symbol_table;
    symbol_table.create_variable("testA");
    symbol_table.create_variable("testB");
    symbol_table.create_variable("C");

    expression_t expression;
    expression.register_symbol_table(symbol_table);

    parser_t parser;
    parser.compile(expression_string, expression);

    printf(" # | A | B | C | %s\n"
        "---+---+---+---+-%s\n",
        expression_string.c_str(),
        std::string(expression_string.size(), '-').c_str());

    for (int i = 0; i < 8; ++i)
    {
        symbol_table.get_variable("testA")->ref() = T((i & 0x01) ? 1 : 0);
        symbol_table.get_variable("testB")->ref() = T((i & 0x02) ? 1 : 0);
        symbol_table.get_variable("C")->ref() = T((i & 0x04) ? 1 : 0);

        

        const int result = static_cast<int>(expression.value());

        printf(" %d | %d | %d | %d | %d \n",
            i,
            static_cast<int>(symbol_table.get_variable("testA")->value()),
            static_cast<int>(symbol_table.get_variable("testB")->value()),
            static_cast<int>(symbol_table.get_variable("C")->value()),
            result);
    }
}

int main()
{
    logic<double>();
    return 0;
}
