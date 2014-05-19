#ifndef TOKEN_H
#define TOKEN_H

#include "common.h"

class Token {
    string str, current;
    string::iterator idx, prevIdx;
public:
    enum TokType {STRING, DELIM, KEYWORD, NUMBER, IDENT, EOS};

    Token(string src) : str(src) {
        Next();
        idx = str.begin();
        prevIdx = idx;
    }

    TokType Type () {
        return type;
    }

    string Next() {
        try {
            prevIdx = idx;
            current = nextInternal();
            return current;
        }
        catch(...) {
            type = EOS;
            return "";
        }
    }

    string Tail() {
        string result = "";
        auto i = idx;
        for(; i != str.end(); ++i) {
            result += *i;
        }

        return result;
    }

    string PushBack() {
        idx = prevIdx;
        return current;
    }

    string Current() {
        return current;
    }

    operator string() {
        return current;
    }

private:
    TokType type;

    string nextInternal() {
        string result = "";

        while(strchr("\r\n\t ", *idx)) ++idx;

        if(isalpha(*idx)) {
            result += *idx;
            while(isalpha(*(++idx)) || (*idx >= '0' && *idx <= '9') || *idx == '@' || *idx == '_')
                result += *idx;
            type = IDENT;
        }
        else if(*idx >= '0' && *idx <= '9') {
            result += *idx;
            bool hasDot = false;
            while ((*(++idx) >= '0' && *idx <= '9') ||
                   [&](){
                   if(!hasDot && *idx == '"')
                        return true;
                   hasDot = true;
                   return false;
                }())
                result += *idx;
            type = NUMBER;
        }
        else if(*idx == '-' && *(idx + 1) == '>') {
            idx += 2;
            type = DELIM;
            result = "->";
        }
        else if(strchr("[]{}()=-+", *idx)) {
            result = *idx;
            ++idx;
            type = DELIM;
        }
        else if(*idx == '"') {
            while(*(++idx) != '"')
                result += *idx;
        }
        else {
            throw "Unknown operator";
        }
        return result;
    }
};

#endif // TOKEN_H
