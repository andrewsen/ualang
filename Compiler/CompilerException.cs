//
//  CompilerException.cs
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

namespace Translator
{
    internal enum ExceptionType
    {
        UnknownOp,

        BadExpression,

        Import,

        FlowError,

        lValueExpected,

        WhileExpected,

        Brace, 
        IllegalToken,
        IllegalCast,
        ComaExpected,
        IllegalType,
        EosExpexted,
        ExcessToken,
        NonNumericValue
    }

    internal class CompilerException : Exception
    {
        private string ex;
        private ExceptionType type;
        private TokenStream toks;

        public CompilerException(ExceptionType t, string ex, TokenStream toks)
        {
            this.type = t;
            this.toks = toks;
            this.ex = ex;
        }

        public override string Message
        {
            get
            {
                return ex;
            }
        }

        public int Where
        {
            get
            {
                if (toks != null)
                    return toks.Line;
                else
                    return -1;
            }
        }

        public string What
        {
            get
            {
                if (toks != null)
                    return toks.ToString() + " " + toks.CodeLine;
                else
                    return "";
            }
        }

        public ExceptionType Type
        {
            get
            {
                return type;
            }
        }

    }
}
