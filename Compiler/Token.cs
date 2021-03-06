﻿//
//  Token.cs
//
//  Author:
//       Andrew Senko <andrewsen98@gmail.com>
//
//  Copyright (c) 2014 Andrew Senko
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace Translator
{
    enum TokenType { Identifier, Keyword, Delimiter, OperatorAssign, Operator, String, Digit, Unknown, Char, EOF, Endl, Separator, Boolean }

    class TokenStream
    {
        string codeLine;
        public string Source;
        int pos, posb;
        string toks = "";
        int line = 1;
        StreamReader reader = null;
        //bool in_string = false;
        bool skipEndl = true, noQuotes = false;
        TokenType type;

        public bool NoQuotes
        {
            get
            {
                return noQuotes;
            }
            set
            {
                noQuotes = value;
            }
        }

        public string CodeLine
        {
            get
            {
                return codeLine;
            }
        }

        public bool SkipEndl
        {
            get { return skipEndl; }
            set { skipEndl = value; }
        }


        public TokenType Type
        {
            get { return type; }
            private set { type = value; }
        }

        public int Position
        {
            get { 
                if (reader == null)
                    return pos;
                else
                    return (int)reader.BaseStream.Position;
                
            }
            set { 
                if (reader == null)
                    pos = value;
                else
                    reader.BaseStream.Position = value;
            }
        }

        public int Line
        {
            get { return line; }
            private set { line = value; }
        }

        public TokenStream(ref string str)
        {
            Source = str + " ";
            pos = 0;
        }

        public TokenStream(StreamReader reader) {
            this.reader = reader;
        }

        public TokenStream(string str)
        {
            Source = str + " ";
            pos = 0;
        }

        public void PushBack() {
            pos = posb;
        }

        public string Next()
        {
            toks = "";
            try
            {
                string res = nextInternal();
                if ((res == "true" || res == "false") && type == TokenType.Identifier) type = TokenType.Boolean;
                return res;
            }
            catch (IndexOutOfRangeException)
            {
                type = TokenType.EOF;
                return null;
            }
        }

        private bool isIdent(char ch)
        {
            if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= 'а' && ch <= 'я') 
                || (ch >= 'А' && ch <= 'Я') || (ch == '_') || (ch == 'і') || (ch == 'І') || (ch == 'ї') || (ch == 'Ї')
                || (ch == 'ґ') || (ch == 'Ґ') || (ch == 'є') || (ch == 'Є')) return true;
            else if(ch >= '0' && ch <= '9') return true;
            else return false;
        }

        private string nextInternal () {
            posb = pos;

            if (pos >= Source.Length)
            {
                type = TokenType.EOF;
                return null;
            }
            char ch = Source[pos];
            codeLine += ch;
            if(skipEndl)
                while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
                {
                    ch = Source[++pos];
                    codeLine += ch;
                    if (ch != '\n') continue;
                    ++line;
                    codeLine = "";
                }
            else
                while (ch == ' ' || ch == '\t')
                {
                    ch = Source[++pos];
                    codeLine += ch;
                }
			

			if (ch == '/' && Source[pos + 1] == '/')
			{
				codeLine += "//";
				++pos;
				//Console.WriteLine("Comment!");
				while (pos < Source.Length - 1)
				{
					codeLine += Source[pos];
					if (Source[++pos] == '\r' || Source[pos] == '\n') break;
				}
				return nextInternal ();
			}
			if (ch == '/' && Source[pos + 1] == '*')
			{
				++pos;
				while (pos < Source.Length - 1)
				{
					codeLine += Source[pos];
					if (Source [++pos] == '*' && Source [pos + 1] == '/') {
						pos += 2;
						break;
					}
				}
				return nextInternal ();
			}

            if (!skipEndl && ch == '\n' || (ch == '\r' && Source[pos + 1] == '\n'))
            {
                ++line;
                pos += 2;
                toks = "\r\n";
                codeLine += toks;
                type = TokenType.Endl;
                return toks;
            }
            if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= 'а' && ch <= 'я') 
                || (ch >= 'А' && ch <= 'Я') || (ch == '_') || (ch == 'і') || (ch == 'І') || (ch == 'ї') || (ch == 'Ї')
                || (ch == 'ґ') || (ch == 'Ґ') || (ch == 'є') || (ch == 'Є'))
            {
                toks += ch;
                while (isIdent(Source[++pos]) || (Source[pos] == '@') && pos != 0 && isIdent(Source[pos-1]))
                    toks += Source[pos];
                type = TokenType.Identifier;
                codeLine += toks;
                return toks;
            }

            if (ch >= '0' && ch <= '9')
            {
                bool dot = false;
                toks += ch;
                while (true)
                {
                    if (Source[++pos] >= '0' && Source[pos] <= '9')
                        toks += Source[pos];
                    else if (!dot && Source[pos] == '.')
                    {
                        toks += '.';
                        dot = true;
                    }
                    else break;
                }
                codeLine += toks;
                if (toks.EndsWith(".")) type = TokenType.Unknown;
                else type = TokenType.Digit;
                return toks;
            }
            if ("{}[]()?:.,@".IndexOf(ch) != -1)
            {
                toks += ch;
                type = TokenType.Delimiter;
                ++pos;
                codeLine += toks;
                return toks;
            }
            if (ch == ';')
            {
                toks += ch;
                type = TokenType.Separator;
                ++pos;
                codeLine += toks;
                return toks;
            }
            if("-+*/|~^&%!<>=".IndexOf(ch) != -1) {
                ++pos;
                string temp = "" + ch + Source[pos];
                if (Source[pos] == '=')
                {
                    toks += ch + "" + Source[pos];
                    type = TokenType.OperatorAssign;
                    ++pos;
                }
                else if (temp == "||" || temp == "&&" || temp == "++" || temp == "--")
                {
                    toks = temp;
                    type = TokenType.Operator;
                    ++pos;
                }
                else
                {
                    toks += ch;
                    type = TokenType.Operator;
                }
                codeLine += toks;
                return toks;
            }
            if (ch == '"')
            {
                while (Source[++pos] != '"')
                    toks += Source[pos];
                ++pos;
                type = TokenType.String;
                if (!noQuotes) toks = "\"" + toks + "\"";
                codeLine += toks;
                return toks;
            }
            if (ch == '\'')
            {
                while (Source[++pos] != '\'')
                    toks += Source[pos];
                ++pos;
                type = TokenType.Char;
                codeLine += toks;
                return toks;
            }
            return null;
        }

        public override string ToString()
        {
            return toks;
        }

        /*public string Source { 
            get 
            {
                return this.code;
            }
            set
            {
                this.code = value;
            }
        }*/
    }
}
/* 
 * // [0] - program stack
 * // [1] - array stack
 * // [2] - call stack
 * // [3] - instruction stack
 * 
 * .import "std"
 * .import "std.runtime"
 * .using Std::
 * .using Std::Runtime::
 * 
 * prot int main (int, char**)
 *
 * prot void ftest (void)
 * 
 * fun int main (int, char**)
 *      p2ui [1]
 *      mov vra, vr1
 *      add vr1, 1
 *      ui2p vra
 *      push vra
 *      call printf (char*, ...)
 *      pop
 *      pop
 *      push vr1
 *      call printf (char*, ..)
 *      pop
 *      pop
 *      call ftest (void)
 *      ret 0
 * end
 * 
 * fun void ftest (void)
 *      push "test.mod"
 *      call ldmod (char*)
 *      swst [0]
 *      
 * end
 * 
 *  _____________________________
 * |    |ch**|    |    |    |    |
 * 
 * 
 * 
 * import Std;
 * 
 * using Std;
 * 
 * int main(int argc, char** argv) {
 * 
 * }
 * 
 * void ftest () {
 * 
 * }
 * 
 */