//
//  AtributteReader.cs
//
//  Author:
//       Andrew Senko <andrewsen98@gmail.com>
//
//  Copyright (c) 2015 Andrew Senko
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
using Translator;
using CompilerClasses;
using System.Globalization;

namespace Compiler
{
    public struct AttributeData
    {
        internal DataTypes Type;
        internal object Value;
        internal bool IsOptional;
        internal string Key;
    }

    public struct AttributeObject
    {
        internal string Name;
        internal bool Binded;
        internal List<AttributeData> Data;
    }

    public class AttributeReader
    {
        private TokenStream _ts;
        private AttributeObject _aobj = new AttributeObject();

        internal AttributeReader(TokenStream ts)
        {
            _ts = ts;
        }

        public AttributeObject Read()
        {
            _aobj = read();
            return _aobj;
        }

        private AttributeObject read()
        {
            _aobj.Binded = false;
            _aobj.Data = new List<AttributeData>();

            _aobj.Name = _ts.Next();
            if (_ts.Type != TokenType.Identifier)
                throw new AttributeException(_aobj.Name, "Wrong name");
            if (_ts.Next() == ";")
                return _aobj;
            else if (_ts.ToString() != "(")
            {
                _aobj.Binded = true;
                _ts.PushBack();
                return _aobj;
            }

            if (_ts.Next() == ")")
            {
                if (_ts.Next() == ";")
                    return _aobj;
                _aobj.Binded = true;
                _ts.PushBack();
                return _aobj;
            }

            while (true)
            {
                AttributeData ad = new AttributeData();
                ad.IsOptional = false;

                var type = _ts.Type;
                var val = _ts.ToString();

                if(type == TokenType.Identifier) //For smth like @Attr(<Key>=<Value>)
                {
                    if (_ts.Next() == "=")
                    {
                        ad.IsOptional = true;
                        ad.Key = val;
                        val = _ts.Next();
                        type = _ts.Type;
                    }
                    else //For @Attr(<Value>)
                        _ts.PushBack();
                }

                switch (type) {
                    case TokenType.Boolean:
                        ad.Value = bool.Parse(val);
                        ad.Type = DataTypes.Bool;
                        break;
                    case TokenType.Digit:
                        if(val.Contains("."))
                        {
                            ad.Value = double.Parse(val, System.Globalization.NumberStyles.Any, CultureInfo.InvariantCulture);
                            ad.Type = DataTypes.Double;
                        }
                        else
                        {
                            ad.Value = int.Parse(val);
                            ad.Type = DataTypes.Int;
                        }
                        break;
                    case TokenType.String:
                        string v = val.Remove(0,1);
                        ad.Value = v.Remove(v.LastIndexOf('"')).Replace("\\\"", "\"");
                        ad.Type = DataTypes.String;
                        break;
                    default:
                        throw new AttributeException(_aobj.Name, "Unsupported data type: " + type.ToString().Replace("DataTypes.",""));
                        break;
                }

                _aobj.Data.Add(ad);

                if(_ts.Next() == ")")
                    break;
                else if(_ts.ToString() != ",")
                    throw new AttributeException(_aobj.Name, "Unexpected character " + _ts.ToString());
                _ts.Next();

            }
            if (_ts.Next() == ";")
                return _aobj;
            _aobj.Binded = true;
            _ts.PushBack();
            return _aobj;
        }
    }
}

