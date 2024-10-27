/*
 * Copyright (C) Rida Bazzi, 2016
 *
 * Do not share this file with anyone
 */
#include <iostream>
#include <istream>
#include <vector>
#include <string>
#include <cctype>

#include "lexer.h"
#include "inputbuf.h"

using namespace std;

string reserved[] = { "END_OF_FILE",
    "IF", "WHILE", "DO", "THEN", "PRINT",
    "PLUS", "MINUS", "DIV", "MULT",
    "EQUAL", "COLON", "COMMA", "SEMICOLON",
    "LBRAC", "RBRAC", "LPAREN", "RPAREN",
    "NOTEQUAL", "GREATER", "LESS", "LTEQ", "GTEQ",
    "DOT", "NUM", "ID",  "REALNUM", "BASE08NUM", "BASE16NUM","ERROR"
};

#define KEYWORDS_COUNT 5
#define NUMTYPE_COUNT 3
string keyword[] = { "IF", "WHILE", "DO", "THEN", "PRINT" };

const string DOTSTRING = ".";
const string BASE08STRING = "x08";
const string BASE16STRING = "x16";
const string EMPTYSTRING = "";

struct NumberType {
    const string validate_string;
    int base;
    TokenType tokenType;
};

static const NumberType numberTypes[] = {
    { DOTSTRING, 10, REALNUM },
    { BASE08STRING, 8, BASE08NUM },
    { BASE16STRING, 16, BASE16NUM },
};

void Token::Print()
{
    cout << "{" << this->lexeme << " , "
         << reserved[(int) this->token_type] << " , "
         << this->line_no << "}\n";
}

LexicalAnalyzer::LexicalAnalyzer()
{
    this->line_no = 1;
    tmp.lexeme = EMPTYSTRING;
    tmp.line_no = 1;
    tmp.token_type = ERROR;
}

bool LexicalAnalyzer::SkipSpace()
{
    char c;
    bool space_encountered = false;

    input.GetChar(c);
    line_no += (c == '\n');

    while (!input.EndOfInput() && isspace(c)) {
        space_encountered = true;
        input.GetChar(c);
        line_no += (c == '\n');
    }

    if (!input.EndOfInput()) {
        input.UngetChar(c);
    }
    return space_encountered;
}

bool LexicalAnalyzer::IsKeyword(string s)
{
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (s == keyword[i]) {
            return true;
        }
    }
    return false;
}

TokenType LexicalAnalyzer::FindKeywordIndex(string s)
{
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (s == keyword[i]) {
            return (TokenType) (i + 1);
        }
    }
    return ERROR;
}

bool LexicalAnalyzer::verifyPatternMatch(const string& s) 
{
    char c;
    string collected_chars;

    for (int i = 0; i < s.length(); ++i) {
        char expectedChar = s[i];

        if (input.EndOfInput()) {
            input.UngetString(collected_chars);
            return false;
        }

        input.GetChar(c);
        collected_chars += c;

        if (c != expectedChar) {
            input.UngetString(collected_chars);
            return false;
        }
    }

    return true;
}


bool isDigitInBase(char c, int base = 10) {
    if (base <= 10) {
        return c >= '0' && c < '0' + base;
    } else {
        return (c >= '0' && c <= '9') || (toupper(c) >= 'A' && toupper(c) < 'A' + base - 10);
    }
}

bool isPositiveDigitInBase(char c, int base = 10) {
    if (base <= 10) {
        return c >= '1' && c < '0' + base;
    } else {
        return (c >= '1' && c <= '9') || (toupper(c) >= 'A' && toupper(c) < 'A' + base - 10);
    }
}

Token LexicalAnalyzer::ScanNumber() 
{
    string lexeme;

    tmp.lexeme = EMPTYSTRING;
    tmp.token_type = ERROR;
    tmp.line_no = line_no;

    for (int i = 0; i < NUMTYPE_COUNT; i++) {
        const NumberType numberType = numberTypes[i];
        lexeme = matchToBase(numberType.validate_string, numberType.base);
        if (!lexeme.empty()) {
            tmp.lexeme = lexeme;
            tmp.token_type = numberType.tokenType;
            return tmp;
        }
    }

    lexeme = checkAndReturnNum();
    if (!lexeme.empty()) {
        tmp.lexeme = lexeme;
        tmp.token_type = NUM;
        return tmp;
    }

    return tmp;
}

string LexicalAnalyzer::checkAndReturnNum()
{
    char c;
    string scanned = EMPTYSTRING;

    input.GetChar(c);
    scanned += c;
    if (isdigit(c)) {
        if (c == '0') {
            return scanned;
        } else {
            input.GetChar(c);
            while (!input.EndOfInput() && isdigit(c)) {
                scanned += c;
                input.GetChar(c);
            }
            if (!input.EndOfInput()) {
                input.UngetChar(c);
            }
            return scanned;
        }
    } 
    input.UngetString(scanned);
    return EMPTYSTRING;
}

string LexicalAnalyzer::matchToBase(const string& validate_string, int base)
{
    char c; 
    string scanned = EMPTYSTRING;
    input.GetChar(c);
    scanned += c;   

    if (c == '0') {
        if (!verifyPatternMatch(validate_string)) {
            input.UngetString(scanned);
            return EMPTYSTRING;
        }
        scanned += validate_string;

        if (validate_string != DOTSTRING)
            return scanned;

        input.GetChar(c);
        while (!input.EndOfInput() && c == '0') {
            scanned += c;
            input.GetChar(c);

            if (c == '\n') {
                scanned += c;
                input.UngetString(scanned);
                return EMPTYSTRING;
            }
        }

        if (!input.EndOfInput() && isPositiveDigitInBase(c, base)) {
            while (!input.EndOfInput() && isDigitInBase(c, base)) {
                scanned += c; 
                input.GetChar(c);
            }
            if (!input.EndOfInput())
                input.UngetChar(c);
        }

        return scanned;
    }

    if (isPositiveDigitInBase(c, base)) {
        input.GetChar(c);
        while (!input.EndOfInput() && isDigitInBase(c, base)) {
            scanned += c;
            input.GetChar(c);
        }
        if (!input.EndOfInput())
            input.UngetChar(c);

        if (!verifyPatternMatch(validate_string)) {
            input.UngetString(scanned);
            return EMPTYSTRING;
        }

        scanned += validate_string;

        if (validate_string != DOTSTRING) 
            return scanned;

        input.GetChar(c);
        bool after_dot_flag = false;
        while (!input.EndOfInput() && isDigitInBase(c, base)) {
            after_dot_flag = true;
            scanned += c;
            input.GetChar(c);
        }
        if (!input.EndOfInput()) 
            input.UngetChar(c);

        if (after_dot_flag)
            return scanned;
    }

    input.UngetString(scanned);
    return EMPTYSTRING;
}

Token LexicalAnalyzer::ScanIdOrKeyword()
{
    char c;
    input.GetChar(c);

    if (isalpha(c)) {
        tmp.lexeme = EMPTYSTRING;
        while (!input.EndOfInput() && isalnum(c)) {
            tmp.lexeme += c;
            input.GetChar(c);
        }
        if (!input.EndOfInput()) {
            input.UngetChar(c);
        }
        tmp.line_no = line_no;
        if (IsKeyword(tmp.lexeme))
            tmp.token_type = FindKeywordIndex(tmp.lexeme);
        else
            tmp.token_type = ID;
    } else {
        if (!input.EndOfInput()) {
            input.UngetChar(c);
        }
        tmp.lexeme = EMPTYSTRING;
        tmp.token_type = ERROR;
    }
    return tmp;
}

TokenType LexicalAnalyzer::UngetToken(Token tok)
{
    tokens.push_back(tok);;
    return tok.token_type;
}

Token LexicalAnalyzer::GetToken()
{
    char c;

    if (!tokens.empty()) {
        tmp = tokens.back();
        tokens.pop_back();
        return tmp;
    }

    SkipSpace();
    tmp.lexeme = EMPTYSTRING;
    tmp.line_no = line_no;
    input.GetChar(c);
    switch (c) {
        case '.':
            tmp.token_type = DOT;
            return tmp;
        case '+':
            tmp.token_type = PLUS;
            return tmp;
        case '-':
            tmp.token_type = MINUS;
            return tmp;
        case '/':
            tmp.token_type = DIV;
            return tmp;
        case '*':
            tmp.token_type = MULT;
            return tmp;
        case '=':
            tmp.token_type = EQUAL;
            return tmp;
        case ':':
            tmp.token_type = COLON;
            return tmp;
        case ',':
            tmp.token_type = COMMA;
            return tmp;
        case ';':
            tmp.token_type = SEMICOLON;
            return tmp;
        case '[':
            tmp.token_type = LBRAC;
            return tmp;
        case ']':
            tmp.token_type = RBRAC;
            return tmp;
        case '(':
            tmp.token_type = LPAREN;
            return tmp;
        case ')':
            tmp.token_type = RPAREN;
            return tmp;
        case '<':
            input.GetChar(c);
            if (c == '=') {
                tmp.token_type = LTEQ;
            } else if (c == '>') {
                tmp.token_type = NOTEQUAL;
            } else {
                if (!input.EndOfInput()) {
                    input.UngetChar(c);
                }
                tmp.token_type = LESS;
            }
            return tmp;
        case '>':
            input.GetChar(c);
            if (c == '=') {
                tmp.token_type = GTEQ;
            } else {
                if (!input.EndOfInput()) {
                    input.UngetChar(c);
                }
                tmp.token_type = GREATER;
            }
            return tmp;
        default:
            if (isdigit(c)) {
                input.UngetChar(c);
                return ScanNumber();
            } else if (isalpha(c)) {
                input.UngetChar(c);
                return ScanIdOrKeyword();
            } else if (input.EndOfInput())
                tmp.token_type = END_OF_FILE;
            else
                tmp.token_type = ERROR;

            return tmp;
    }
}

int main()
{
    LexicalAnalyzer lexer;
    Token token;

    token = lexer.GetToken();
    token.Print();
    while (token.token_type != END_OF_FILE)
    {
        token = lexer.GetToken();
        token.Print();
    }
}
