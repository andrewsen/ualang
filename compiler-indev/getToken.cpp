/*
    uac -- bilexical compiler. A part of bilexical programming language gramework
    Copyright (C) 2013-2014 Andrew Senko.

    This file is part of uac.

    uac is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    uac is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with uac.  If not, see <http://www.gnu.org/licenses/>.
*/

/*  Written by Andrew Senko <andrewsen@yandex.ru>. */

#include "parser_common.h"
#include <sstream>

bool isHexString(const string& str);
bool isOctString(const string& str);
bool isBinString(const string& str);

Token::Token(string source) {
	//this->noDir = noDir;
	this->token = source;
    this->tokenIndex = 0;
    this->ptr = token.begin();
    //strcpy(this->ptr, source.c_str());
	curToken = "";
    this->prevIndex = token.begin();
	firstTime = true;
}

string Token::GetNextToken() {
	curToken = getNexToken_priv();
	firstTime = false;
	//cout << "Args parsed!" << endl;
	return curToken;
}

string Token::GetCurrentToken() {
	return curToken;
}

string Token::getNexToken_priv() {
    this->prevIndex = this->ptr;

    string tok = "";
    while(isspace(*ptr) && *ptr) {
        ptr++;
    }

    if(!*ptr) {
        //cout << "EOF: '" << *ptr << "'" << endl;
        type = Token::TokenType::EOF;
        return "EOF";
    }

    if(isdigit(*ptr)) {
        //cout << "Digit: '" << *ptr << "'" << endl;
        tok += *ptr;
        //cout << tok << endl;
        ptr++;
        while (!strchr(" ,;:+-=[](){}\r\n", *ptr) && *ptr) {
            //cout << "digit: " << *ptr << endl << "\tdec: " << (int)*ptr << endl;
            tok += *ptr;
            ptr++;
        }
        if(tok[0] == '0' && tok[1] == 'x') {
            //cout << atoi("AF") << endl;
            cout << "HEX token: " << tok << endl;
            tok.erase(tok.begin(), tok.begin()+2);
            if(!isHexString(tok)) {
                this->token = tok;
                type = Token::TokenType::UNKNOWN;
                tokenIndex++;
                return tok;
            }
            unsigned int d;
            std::stringstream ss;
            ss << std::hex << tok;
            ss >> d;
            std::stringstream ss2;
            ss2 << std::dec << d;
            ss2 >> tok;
            //cout << "DEC digit: " << tok << endl;
        }
        else if(tok[0] == '0' && tok[1] == 'b') {
            //cout << atoi("AF") << endl;
            //cout << "BIN token: " << tok << endl;
            tok.erase(tok.begin(), tok.begin()+2);
            if(!isBinString(tok)) {
                this->token = tok;
                type = Token::TokenType::UNKNOWN;
                tokenIndex++;
                return tok;
            }
            //char * p;
            auto d = strtol(tok.c_str(), NULL, 2);
            stringstream ss;
            ss << std::dec << d;
            ss >> tok;
            //cout << "DEC digit: " << tok << endl;
        }
        else if(tok[0] == '0' && tok.size() > 1) {
            //cout << atoi("AF") << endl;
            //cout << "OCT token: " << tok << endl;
            tok.erase(tok.begin(), tok.begin()+2);
            if(!isOctString(tok)) {
                this->token = tok;
                type = Token::TokenType::UNKNOWN;
                tokenIndex++;
                return tok;
            }
            unsigned int d;
            std::stringstream ss;
            ss << std::oct << tok;
            ss >> d;
            std::stringstream ss2;
            ss2 << std::dec << d;
            ss2 >> tok;
            //cout << "DEC digit: " << tok << endl;
        }
        this->token = tok;
        //cout << "~~~DIGIT: " << tok << endl;
        if(is_digits_only(tok.c_str())) type = Token::TokenType::DIGIT;
        else type = Token::TokenType::UNKNOWN;
        //cout << tok;
        tokenIndex++;
        //cout << "return" << endl;
        //cout << "DEC PTR AFTER " << tok << " = " << *ptr << endl;
        return tok;
    }
    if(*ptr == '\"') {
        ptr++;
        bool hasSlash = false;
        while (*ptr && *ptr != '\"') {
            if (hasSlash)
            {
                switch (*ptr)
                {
                case 'n':
                    tok.push_back('\n');
                    break;
                case 'r':
                    tok.push_back('\r');
                    break;
                case 't':
                    tok.push_back('\t');
                    break;
                case '0':
                    tok.push_back('\0');
                    break;
                case 'b':
                    tok.push_back('\b');
                    break;
                case 'v':
                    tok.push_back('\v');
                    break;
                case 'a':
                    tok.push_back('\a');
                    break;
                case '\\':
                    tok.push_back('\\');
                    hasSlash = false;
                    break;
                default:
                    throw string("Unknown control symbol: ") + *ptr + "\n";
                    break;
                }
                hasSlash = false;
                //cout << "CS: \\" << *ptr << endl;
            }
            else if(*ptr == '\\') {
                hasSlash = true;
            }
            else tok += *ptr;
            ptr++;
        }
        ptr++;
        tokenIndex++;
        type = Token::TokenType::STRING;
        return tok;
    }

    if(strchr(",:+-=[]{}()", *ptr)) {
        type = Token::TokenType::DELIMETER;
        tokenIndex++;
        auto s = string("") += *ptr;
        ptr++;
        return s;
    }
    if(*ptr == ';') {
        ptr++;
        while (*ptr) {
            tok += *ptr;
            ptr++;
        }
        ptr++;
        tokenIndex++;
        type = Token::TokenType::COMMENT;
        return tok;
    }
    if(*ptr == '/' && *(ptr + 1)) {
        ptr += 2;
        while (*ptr) {
            tok += *ptr;
            ptr++;
        }
        ptr++;
        tokenIndex++;
        type = Token::TokenType::COMMENT;
        return tok;
    }
    if(*ptr == ';') {
        ptr++;
        while (*ptr) {
            tok += *ptr;
            ptr++;
        }
        ptr++;
        tokenIndex++;
        type = Token::TokenType::COMMENT;
        return tok;
    }
    if(*ptr == '.' && firstTime) {
        //cout << "Directive dot: '" << *ptr << "'" << endl;
        tok += *ptr;
        ptr++;
        while(IsCharAlpha(*ptr) || isdigit(*ptr) || *ptr == '_') {
            tok += *ptr;
            ptr++;
        }
        tokenIndex++;
        type = Token::TokenType::DIRECTIVE;
        return tok;
    }
    if(*ptr == '.' && !firstTime) {
        tok = ".";
        ptr++;
        tokenIndex++;
        type = Token::TokenType::DELIMETER;
        return tok;
    }
    if(IsCharAlpha(*ptr) || *ptr == '_' || *ptr == '$') {
        //cout << "Alpha: '" << *ptr << "'" << endl;
        tok += *ptr;
        ptr++;
        while(IsCharAlpha(*ptr) || isdigit(*ptr) || *ptr == '_' || *ptr == '&') {
            tok += *ptr;
            ptr++;
        }
        //cout << "Token: " << tok << endl;
        tokenIndex++;
        type = Token::TokenType::IDENTIFIER;
        return tok;
    }
    tokenIndex++;
    type = Token::TokenType::UNKNOWN;
    //cout << "Unknown character: '" << *ptr << "'" << endl;
    return "";
}

void Token::PushBack() {
    if(this->prevIndex == token.begin()) return;
    this->ptr = this->prevIndex;
    this->prevIndex = token.begin();
}

int Token::GetTokenIndex() {
	return this->tokenIndex;
}

Token::TokenType Token::GetTokenType() {
	return this->type;
}

string Token::GetTail() {
    string tail;
    auto p = ptr;
    while (*p)
    {
        tail += *p;
        ++p;
    }
	return tail;
}


// Output a string.
/*ostream &operator<<(ostream &stream, string &o)
{
  stream << o.p.toStdString();
  return stream;
}

// Input a string.
istream &operator>>(istream &stream, string &o)
{
	//stream >> o.p;
	char t[255]; // arbitrary size - change if necessary
  int len;

  stream.getline(t, 255);
  len = strlen(t) + 1;

  o.p = t;
  return stream;
}

string operator+(const char *s, const string &o)
{
    string temp = o.p + s;
  return temp;
}*/

string asciiToUnicode(const char * str) {
	int len = strlen(str);

    string ret = "";

	union transfom {
		char wch;
		char ch;
	};

	for (int i = 0; i < len; i++) {
		transfom tr;
		tr.wch = 0;
		tr.ch = str[i];
		ret += tr.wch;
	}
	return ret;
}

int isUnicode = -1;

bool IsCharAlpha(int wch) {

    //cout << (int)(unsigned char) wch << endl;
    int uch = (int)(unsigned char)wch;
    if(isUnicode != -1) {
        if(isUnicode == 208) {
            isUnicode = -1;
            if(uch >= 144 || uch <= 191) return true;
            //cout << "Вхід symbols detected!" << endl;
        }
        if(isUnicode == 209) {
            isUnicode = -1;
            if(uch >= 128 || uch <= 143) return true;
            //cout << "Вхід symbols detected!" << endl;
        }
    }
    if(wch >= 'a' && wch <= 'z') return true;
    else if(wch >= 'A' && wch <= 'Z') return true;
    else if((int)(unsigned char)wch == 209 || (int)(unsigned char)wch ==208) {
        //cout << "Unicode special: " << wch << endl;
        isUnicode = (int)(unsigned char)wch ;
        return true;
    }
    else if(wch >= 'а' && wch <= 'я') return true;
    else if(wch >= 'А' && wch <= 'Я') return true;
    else if(wch == 'і' || wch == 'І' || wch == 'є' || wch == 'Є') return true;
    else if(wch == 'в' || wch == 'х' || wch == 'і' || wch == 'д') return true;
    else if(wch == 'В' || wch == 'Х' || wch == 'І' || wch == 'Д') return true;
    else if(isalpha(wch)) return true;
    else return false;
}

//
//// Output a string.
//ostream &operator<<(ostream &stream, string &o)
//{
//  stream << o.p;
//  return stream;
//}
//
//// Input a string.
//istream &operator>>(istream &stream, string &o)
//{
//  char t[255]; // arbitrary size - change if necessary
//  int len;
//
//  stream.getline(t, 255);
//  len = strlen(t) + 1;
//
//  if(len > o.size) {
//    //delete [] o.p;
//    try {
//      o.p = new char[len];
//    } catch (bad_alloc xa) {
//      cout << "Allocation error\n";
//      exit(1);
//    }
//    o.size = len;
//  }
//  strcpy(o.p, t);
//  return stream;
//}
//
//string operator+(const char *s, const string &o)
//{
//  int len;
//  string temp;
//
//  //delete [] temp.p;
//
//  len = strlen(s) + strlen(o.p) + 1;
//  temp.size = len;
//  try {
//    temp.p = new char[len];
//  } catch (bad_alloc xa) {
//    cout << "Allocation error\n";
//    exit(1);
//  }
//  strcpy(temp.p, s);
//
//  strcat(temp.p, o.p);
//
//  return temp;
//}

bool endsWith(string s, const string &str) {
    return (s, str, false);
}

bool endsWith(string s, const string &str, bool ignoreCase) {
    if(!ignoreCase) {
        if(s.find_last_of(str) == s.length() - str.length()) return true;
    }
    else {
        //if(p.lastIndexOf(str.ToLower()) == s.length() - str.length()) return true;
        if(s.find_last_of(str) == s.length() - str.length()) return true;
    }
    return false;
}
string trim(string str) {
    //return String(p.trimmed());
    /*
    QString t = "";

    //cout << "Before trim '" << p << "'\n";
    bool not_space = false;
    for(QChar ch : this->p) {
        if(!ch.isSpace() || not_space) {
            t += ch;
            not_space = true;
        }
    }
    for(auto i = t.end(); i != t.begin(); i--) {
        if(!isspace(*i)) break;

        t.pop_back();
    }
    //cout << "After trim '" << t << "'\n";
    return String(t);
*/
    int start = 0, end = str.length();
    for (; str[start] != '\0'; start++) {
        if(str[start] != ' ') break;
    }
    for (; end >= 0; end--) {
        if(str[end] != ' ') break;
    }
    //cout <<  end << endl;
    string array;
    //char *array = new char[end-start+2];
    //array[end-start+1] = '\0';
    for (int i = 0; i != end-start+1; i++) {
        //cout << this->cstr[i+start] << endl;
        array += str[i+start];
    }
    //String str = S(array);
    //delete [] array;
    return array;
}

bool isHexString(const string& str) {
    string hexNums = "0123456789AaBbCcDdEeFf";
    for (char ch : str) {
        if(!strchr(hexNums.c_str(), ch)) return false;
    }
    return true;
}

bool isOctString(const string& str) {
    string octNums = "01234567";
    for (char ch : str) {
        if(!strchr(octNums.c_str(), ch)) return false;
    }
    return true;
}

bool isBinString(const string& str) {
    string binNums = "01";
    for (char ch : str) {
        if(!strchr(binNums.c_str(), ch)) return false;
    }
    return true;
}
