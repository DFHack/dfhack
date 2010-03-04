/* Copyright (C) 2004 Xavier Décoret <Xavier.Decoret@imag.fr>
*
* argsteam is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* 
* Foobar is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with Foobar; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
*/

#ifndef ARGSTREAM_H
#define ARGSTREAM_H


#include <string>
#include <list>
#include <deque>
#include <map>
#include <stdexcept>
#include <sstream>
#include <iostream> 

namespace 
{
    class argstream;

    template<class T>
    class ValueHolder;

    template <typename T>
    argstream& operator>> (argstream&, const ValueHolder<T>&);
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Interface of ValueHolder<T>
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    template<class T>
    class ValueHolder
    {
    public:
        ValueHolder(char s,
            const char* l,
            T& b,
            const char* desc,
            bool mandatory);
        ValueHolder(const char* l,
            T& b,
            const char* desc,
            bool mandatory);
        ValueHolder(char s,
            T& b,
            const char* desc,
            bool mandatory);
        friend argstream& operator>><>(argstream& s,const ValueHolder<T>& v);
        std::string name() const;
        std::string description() const;
    private:
        std::string shortName_;
        std::string longName_;
        T*          value_;
        T           initialValue_;
        std::string description_;  
        bool        mandatory_;
    };
    template <class T>
    inline ValueHolder<T>
        parameter(char s,
        const char* l,
        T& b,
        const char* desc="",
        bool mandatory = true)
    {
        return ValueHolder<T>(s,l,b,desc,mandatory);
    }
    template <class T>
    inline ValueHolder<T>
        parameter(char s,
        T& b,
        const char* desc="",
        bool mandatory = true)
    {
        return ValueHolder<T>(s,b,desc,mandatory);
    }
    template <class T>
    inline ValueHolder<T>
        parameter(const char* l,
        T& b,
        const char* desc="",
        bool mandatory = true)
    {
        return ValueHolder<T>(l,b,desc,mandatory);
    }
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Interface of OptionHolder
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    class OptionHolder
    {
    public:
        inline OptionHolder(char s,
            const char* l,
            bool& b,
            const char* desc);
        inline OptionHolder(const char* l,
            bool& b,
            const char* desc);
        inline OptionHolder(char s,
            bool& b,
            const char* desc);
        friend argstream& operator>>(argstream& s,const OptionHolder& v);
        inline std::string name() const;
        inline std::string description() const;
    protected:
        inline OptionHolder(char s,
            const char* l,
            const char* desc);  
        friend OptionHolder help(char s='h',
            const char* l="help",
            const char* desc="Display this help");
    private:
        std::string shortName_;
        std::string longName_;
        bool*       value_;
        std::string description_;  
    };
    inline OptionHolder
        option(char s,
        const char* l,
        bool& b,
        const char* desc="")
    {
        return OptionHolder(s,l,b,desc);
    }
    inline OptionHolder
        option(char s,
        bool& b,
        const char* desc="")
    {
        return OptionHolder(s,b,desc);
    }
    inline OptionHolder
        option(const char* l,
        bool& b,
        const char* desc="")
    {
        return OptionHolder(l,b,desc);
    }
    inline OptionHolder
        help(char s,
        const char* l,
        const char* desc)
    {
        return OptionHolder(s,l,desc);
    }
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Interface of ValuesHolder
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    template<class T,class O>
    class ValuesHolder
    {
    public:
        ValuesHolder(const O& o,
            const char* desc,
            int len);
        friend argstream& operator>><>(argstream& s,const ValuesHolder<T,O>& v);
        std::string name() const;
        std::string description() const;
        typedef T value_type;
    private:
        mutable O   value_;
        std::string description_;  
        int         len_;
        char        letter_;
    };
    template<class T,class O>
    inline ValuesHolder<T,O>
        values(const O& o,
        const char* desc="",
        int len=-1)
    {
        return ValuesHolder<T,O>(o,desc,len);
    }
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Interface of ValueParser
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    template <class T>
    class ValueParser
    {
    public:
        inline T operator()(const std::string& s) const
        {
            std::istringstream is(s);
            T t;
            is>>t;
            return t;
        }
    };
    // We need to specialize for string otherwise parsing of a value that
    // contains space (for example a string with space passed in quotes on the
    // command line) would parse only the first element of the value!!!
    template <>
    class ValueParser<std::string>
    {
    public:
        inline std::string operator()(const std::string& s) const
        {
            return s;
        }
    };  
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Interface of argstream
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    class argstream
    {
    public:
        inline argstream(int argc,char** argv);
        //inline argstream(const char* c);
        template<class T>
        friend argstream& operator>>(argstream& s,const ValueHolder<T>& v);
        friend inline argstream& operator>>(argstream& s,const OptionHolder& v);
        template<class T,class O>
        friend argstream& operator>>(argstream& s,const ValuesHolder<T,O>& v);

        inline bool helpRequested() const;
        inline bool isOk() const;
        inline std::string errorLog() const;
        inline std::string usage() const;
        inline void defaultErrorHandling(bool ignoreUnused=false) const;
        static inline char uniqueLetter();
    protected:
        void parse(int argc,char** argv);
    private:
        typedef std::list<std::string>::iterator value_iterator;
        typedef std::pair<std::string,std::string> help_entry;
        std::string                          progName_;
        std::map<std::string,value_iterator> options_;
        std::list<std::string>               values_;
        bool                                 minusActive_;
        bool                                 isOk_;
        std::deque<help_entry>               argHelps_;
        std::string                          cmdLine_;
        std::deque<std::string>              errors_;
        bool                                 helpRequested_;
    };
    //************************************************************
    // Implementation of ValueHolder<T>
    //************************************************************
    template<class T>
    ValueHolder<T>::ValueHolder(char s,
        const char* l,
        T& v,
        const char* desc,
        bool mandatory)
        :  shortName_(1,s),
        longName_(l),
        value_(&v),
        initialValue_(v),
        description_(desc),
        mandatory_(mandatory)
    {
    }
    template<class T>
    ValueHolder<T>::ValueHolder(const char* l,
        T& v,
        const char* desc,
        bool mandatory)
        :  longName_(l),
        value_(&v),
        initialValue_(v),
        description_(desc),
        mandatory_(mandatory)
    {
    }
    template<class T>
    ValueHolder<T>::ValueHolder(char s,
        T& v,
        const char* desc,
        bool mandatory)
        :  shortName_(1,s),
        value_(&v),
        initialValue_(v),
        description_(desc),
        mandatory_(mandatory)
    {
    }
    template<class T>
    std::string
        ValueHolder<T>::name() const
    {
        std::ostringstream  os;
        if (!shortName_.empty()) os<<'-'<<shortName_;
        if (!longName_.empty()) {      
            if (!shortName_.empty()) os<<'/';
            os<<"--"<<longName_;
        }
        return os.str();
    }
    template<class T>
    std::string
        ValueHolder<T>::description() const
    {
        std::ostringstream  os;  
        os<<description_;
        if (mandatory_)
        {
            os<<"(mandatory)";
        }
        else
        {
            os<<"(default="<<initialValue_<<")";
        }
        return os.str();
    }
    //************************************************************
    // Implementation of OptionHolder
    //************************************************************
    inline OptionHolder::OptionHolder(char s,
        const char* l,
        bool& b,
        const char* desc)
        : shortName_(1,s),
        longName_(l),
        value_(&b),
        description_(desc)
    {
    }
    inline OptionHolder::OptionHolder(const char* l,
        bool& b,
        const char* desc)
        : longName_(l),
        value_(&b),
        description_(desc)
    {
    }
    inline OptionHolder::OptionHolder(char s,
        bool& b,
        const char* desc)
        : shortName_(1,s),
        value_(&b),
        description_(desc)
    {
    }
    inline OptionHolder::OptionHolder(char s,
        const char* l,
        const char* desc)
        : shortName_(1,s),
        longName_(l),
        value_(NULL),
        description_(desc)
    {
    }
    inline std::string
        OptionHolder::name() const
    {
        std::ostringstream  os;
        if (!shortName_.empty()) os<<'-'<<shortName_;
        if (!longName_.empty())
        {
            if (!shortName_.empty()) os<<'/';
            os<<"--"<<longName_;
        }
        return os.str();
    }
    inline std::string
        OptionHolder::description() const
    {
        return description_;
    }
    //************************************************************
    // Implementation of ValuesHolder<T,O>
    //************************************************************
    template<class T,class O>
    ValuesHolder<T,O>::ValuesHolder(const O& o,
        const char* desc,
        int len)
        : value_(o),
        description_(desc),
        len_(len)
    {
        letter_ = argstream::uniqueLetter();
    }
    template <class T,class O>
    std::string
        ValuesHolder<T,O>::name() const
    {
        std::ostringstream  os;
        os<<letter_<<"i";
        return os.str();
    }
    template <class T,class O>
    std::string
        ValuesHolder<T,O>::description() const
    {
        return description_;
    }
    //************************************************************
    // Implementation of argstream
    //************************************************************
    inline
        argstream::argstream(int argc,char** argv)
        : progName_(argv[0]),
        minusActive_(true),
        isOk_(true)
    {
        parse(argc,argv);
    }
    //inline
    //    argstream::argstream(const char* c)
    //    : progName_(""),
    //    minusActive_(true),
    //    isOk_(true)
    //{
    //    std::string s(c);
    //    // Build argc, argv from s. We must add a dummy first element for
    //    // progName because parse() expects it!!
    //    std::deque<std::string> args;
    //    args.push_back("");
    //    std::istringstream is(s);
    //    while (is.good())
    //    {
    //        std::string t;
    //        is>>t;
    //        args.push_back(t);
    //    }
    //    char* pargs[args.size()];
    //    char** p = pargs;
    //    for (std::deque<std::string>::const_iterator
    //        iter = args.begin();
    //        iter != args.end();++iter)
    //    {
    //        *p++ = const_cast<char*>(iter->c_str());
    //    }
    //    parse(args.size(),pargs);
    //}
    inline void
        argstream::parse(int argc,char** argv)
    {
        // Run thru all arguments.
        // * it has -- in front : it is a long name option, if remainder is empty,
        //                        it is an error
        // * it has - in front  : it is a sequence of short name options, if
        //                        remainder is empty, deactivates option (- will
        //                        now be considered a char).
        // * if any other char, or if option was deactivated
        //                      : it is a value. Values are split in parameters
        //                      (immediately follow an option) and pure values.
        // Each time a value is parsed, if the previously parsed argument was an
        // option, then the option is linked to the value in case of it is a
        // option with parameter.  The subtle point is that when several options
        // are given with short names (ex: -abc equivalent to -a -b -c), the last
        // parsed option is -c).
        // Since we use map for option, any successive call overides the previous
        // one: foo -a -b -a hello is equivalent to foo -b -a hello
        // For values it is not true since we might have several times the same
        // value. 
        value_iterator* lastOption = NULL;
        for (char** a = argv,**astop=a+argc;++a!=astop;)
        {
            std::string s(*a);
            if (minusActive_ && s[0] == '-')
            {
                if (s.size() > 1 && s[1] == '-')
                {
                    if (s.size() == 2)
                    {
                        minusActive_ = false;
                        continue;
                    }
                    lastOption = &(options_[s.substr(2)] = values_.end());
                }
                else 
                {
                    if (s.size() > 1)
                    {
                        // Parse all chars, if it is a minus we have an error
                        for (std::string::const_iterator cter = s.begin();
                            ++cter != s.end();)
                        {
                            if (*cter == '-')
                            {
                                isOk_ = false;
                                std::ostringstream os;
                                os<<"- in the middle of a switch "<<a;
                                errors_.push_back(os.str());
                                break;
                            }
                            lastOption = &(options_[std::string(1,*cter)] = values_.end());
                        }
                    }
                    else
                    {
                        isOk_ = false;
                        errors_.push_back("Invalid argument -");
                        break;
                    }
                }
            }
            else
            {
                values_.push_back(s);
                if (lastOption != NULL)
                {
                    *lastOption = --values_.end();
                }
                lastOption = NULL;
            }
        }
#ifdef ARGSTREAM_DEBUG
        for (std::map<std::string,value_iterator>::const_iterator
            iter = options_.begin();iter != options_.end();++iter)
        {
            std::cout<<"DEBUG: option "<<iter->first;
            if (iter->second != values_.end())
            {
                std::cout<<" -> "<<*(iter->second);
            }
            std::cout<<std::endl;
        }
        for (std::list<std::string>::const_iterator
            iter = values_.begin();iter != values_.end();++iter)
        {
            std::cout<<"DEBUG: value  "<<*iter<<std::endl;
        }
#endif // ARGSTREAM_DEBUG
    }
    inline bool
        argstream::isOk() const
    {
        return isOk_;
    }
    inline bool
        argstream::helpRequested() const
    {
        return helpRequested_;
    }
    inline std::string
        argstream::usage() const
    {
        std::ostringstream os;
        os<<"usage: "<<progName_<<cmdLine_<<'\n';
        unsigned int lmax = 0;
        for (std::deque<help_entry>::const_iterator
            iter = argHelps_.begin();iter != argHelps_.end();++iter)
        {
            if (lmax<iter->first.size()) lmax = iter->first.size();
        }  
        for (std::deque<help_entry>::const_iterator
            iter = argHelps_.begin();iter != argHelps_.end();++iter)
        {
            os<<'\t'<<iter->first<<std::string(lmax-iter->first.size(),' ')
                <<" : "<<iter->second<<'\n';
        }
        return os.str();
    }
    inline std::string
        argstream::errorLog() const
    {
        std::string s;
        for(std::deque<std::string>::const_iterator iter = errors_.begin();
            iter != errors_.end();++iter)
        {
            s += *iter;
            s += '\n';
        }
        return s;
    }
    inline char
        argstream::uniqueLetter()
    {
        static unsigned int c = 'a';
        return c++;
    }
    template<class T>
    argstream&
        operator>>(argstream& s,const ValueHolder<T>& v)
    {
        // Search in the options if there is any such option defined either with a
        // short name or a long name. If both are found, only the last one is
        // used.
#ifdef ARGSTREAM_DEBUG    
        std::cout<<"DEBUG: searching "<<v.shortName_<<" "<<v.longName_<<std::endl;
#endif    
        s.argHelps_.push_back(argstream::help_entry(v.name(),v.description()));
        if (v.mandatory_)
        {
            if (!v.shortName_.empty())
            {
                s.cmdLine_ += " -";
                s.cmdLine_ += v.shortName_;
            }
            else
            {
                s.cmdLine_ += " --";
                s.cmdLine_ += v.longName_;
            }
            s.cmdLine_ += " value";
        }
        else
        {
            if (!v.shortName_.empty())
            {
                s.cmdLine_ += " [-";
                s.cmdLine_ += v.shortName_;
            }
            else
            {
                s.cmdLine_ += " [--";
                s.cmdLine_ += v.longName_;
            }  
            s.cmdLine_ += " value]";

        }
        std::map<std::string,argstream::value_iterator>::iterator iter =
            s.options_.find(v.shortName_);
        if (iter == s.options_.end())
        {
            iter = s.options_.find(v.longName_);
        }
        if (iter != s.options_.end())
        {
            // If we find counterpart for value holder on command line, either it
            // has an associated value in which case we assign it, or it has not, in
            // which case we have an error.
            if (iter->second != s.values_.end())
            {
#ifdef ARGSTREAM_DEBUG
                std::cout<<"DEBUG: found value "<<*(iter->second)<<std::endl;
#endif	
                ValueParser<T> p;
                *(v.value_) = p(*(iter->second));
                // The option and its associated value are removed, the subtle thing
                // is that someother options might have this associated value too,
                // which we must invalidate.
                s.values_.erase(iter->second);

                // FIXME this loop seems to crash if a std::string is used as the value
                //for (std::map<std::string,argstream::value_iterator>::iterator
                //    jter = s.options_.begin();jter != s.options_.end();++jter)
                //{
                //    if (jter->second == iter->second)
                //    {
                //        jter->second = s.values_.end();
                //    }
                //}
                s.options_.erase(iter);
            }
            else
            {
                s.isOk_ = false;
                std::ostringstream  os;
                os<<"No value following switch "<<iter->first
                    <<" on command line";
                s.errors_.push_back(os.str());
            }
        }
        else
        {
            if (v.mandatory_)
            {
                s.isOk_ = false;
                std::ostringstream  os;
                os<<"Mandatory parameter ";
                if (!v.shortName_.empty()) os<<'-'<<v.shortName_;
                if (!v.longName_.empty())
                {
                    if (!v.shortName_.empty()) os<<'/';
                    os<<"--"<<v.longName_;
                }
                os<<" missing";
                s.errors_.push_back(os.str());
            }
        }
        return s;
    }
    inline argstream&
        operator>>(argstream& s,const OptionHolder& v)
    {
        // Search in the options if there is any such option defined either with a
        // short name or a long name. If both are found, only the last one is
        // used.
#ifdef ARGSTREAM_DEBUG    
        std::cout<<"DEBUG: searching "<<v.shortName_<<" "<<v.longName_<<std::endl;
#endif
        s.argHelps_.push_back(argstream::help_entry(v.name(),v.description()));
        {
            std::string c;
            if (!v.shortName_.empty())
            {
                c += " [-";
                c += v.shortName_;
            }
            else
            {
                c += " [--";
                c += v.longName_;
            }      
            c += "]";
            s.cmdLine_ = c+s.cmdLine_;
        }
        std::map<std::string,argstream::value_iterator>::iterator iter =
            s.options_.find(v.shortName_);
        if (iter == s.options_.end())
        {
            iter = s.options_.find(v.longName_);
        }
        if (iter != s.options_.end())
        {
            // If we find counterpart for value holder on command line then the
            // option is true and if an associated value was found, it is ignored
            if (v.value_ != NULL)
            {
                *(v.value_) = true;
            }
            else
            {
                s.helpRequested_ = true;
            }
            // The option only is removed
            s.options_.erase(iter);
        }
        else
        {
            if (v.value_ != NULL)
            {
                *(v.value_) = false;
            }
            else
            {
                s.helpRequested_ = false;
            }
        }
        return s;
    }
    template<class T,class O>
    argstream&
        operator>>(argstream& s,const ValuesHolder<T,O>& v)
    {
        s.argHelps_.push_back(argstream::help_entry(v.name(),v.description()));
        {
            std::ostringstream os;
            os<<' '<<v.letter_<<'1';
            switch (v.len_)
            {
            case -1:
                os<<"...";
                break;
            case 1:
                break;
            default:
                os<<"..."<<v.letter_<<v.len_;
                break;
            }
            s.cmdLine_ += os.str();
        }
        std::list<std::string>::iterator first = s.values_.begin();
        // We add to the iterator as much values as we can, limited to the length
        // specified (if different of -1)
        int n = v.len_ != -1?v.len_:s.values_.size();
        while (first != s.values_.end() && n-->0)
        {
            // Read the value from the string *first
            ValueParser<T> p;
            *(v.value_++) = p(*first );
            s.argHelps_.push_back(argstream::help_entry(v.name(),v.description()));
            // The value we just removed was maybe "remembered" by an option so we
            // remove it now.
            for (std::map<std::string,argstream::value_iterator>::iterator
                jter = s.options_.begin();jter != s.options_.end();++jter)
            {
                if (jter->second == first)
                {
                    jter->second = s.values_.end();
                }
            }
            ++first;
        }
        // Check if we have enough values
        if (n != 0)
        {
            s.isOk_ = false;
            std::ostringstream  os;
            os<<"Expecting "<<v.len_<<" values";
            s.errors_.push_back(os.str());
        }
        // Erase the values parsed
        s.values_.erase(s.values_.begin(),first);
        return s;
    }
    inline void
        argstream::defaultErrorHandling(bool ignoreUnused) const
    {
        if (helpRequested_)
        {
            std::cout<<usage();
            exit(1);
        }
        if (!isOk_)
        {
            std::cerr<<errorLog();
            exit(1);
        }
        if (!ignoreUnused &&
            (!values_.empty() || !options_.empty()))
        {
            std::cerr<<"Unused arguments"<<std::endl;
            exit(1);
        }
    }
};
#endif // ARGSTREAM_H

