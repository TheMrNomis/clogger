#include "clogger.h"

/***************************************************
 *                 LoggerSettings                  *
 ***************************************************/

LoggerSettings::LoggerSettings(unsigned int mask, bool showColor, std::ostream & out):
    m_out(out),
    m_printColor(showColor),
    m_mask(mask),
    m_indentWidth(2)
{
}

void LoggerSettings::beginFunction(std::string const& name)
{
    m_stack.push_back(name);
}

void LoggerSettings::endFunction()
{
    if(!m_stack.empty())
        m_stack.pop_back();
}

std::string LoggerSettings::indent() const
{
    return indent(m_stack.size());
}

void LoggerSettings::printStack() const
{
    unsigned int indentLevel = 0;
    for(std::string stackLine : m_stack)
    {
        ++indentLevel;
        Logger() << log_stack << indent(indentLevel) << stackLine;
    }
}

std::string LoggerSettings::indent(unsigned int indentLevel) const
{
    std::stringstream str;
    for(unsigned int i = 0; i < indentLevel; ++i)
        for(unsigned int j = 0; j < m_indentWidth; ++j)
            str << " ";
    return str.str();
}

/***************************************************
 *                     Logger                      *
 ***************************************************/
Logger::Logger(bool endl, LoggerSettings & settings):
    m_settings(settings),
    m_endl(endl),
    m_type(log_log)
{
}

Logger::Logger(LoggerSettings & settings):
    Logger(true, settings)
{
}

Logger::~Logger()
{
    bool printingStack = false;
    std::ostream & out = m_settings.out();
    if(m_type & m_settings.mask())
    {
        switch(m_type)
        {
            case log_warn:
                printColoredMessageHeader(out, LoggerColor::color_yellow, "WARNING");
                printingStack = true;
                break;
            case log_error:
                printColoredMessageHeader(out, LoggerColor::color_red, "ERROR");
                printingStack = true;
                break;
            case log_stack:
                setFGcolor(out, LoggerColor::color_cyan);
                break;
            default:
                if(m_settings.mask() & log_func)
                    m_settings.out() << m_settings.indent();
                break;
        }
        out << m_line.str();
        if(m_endl)
        {
            setFGcolor(out, LoggerColor::color_nocolor);
            out << std::endl;
            if(printingStack)
                m_settings.printStack();
        }
    }
}

/***************************************************
 *                   FuncTracer                    *
 ***************************************************/
FuncTracer::FuncTracer(std::string const& classname, std::string const& funcname, bool addParenthesis, LoggerSettings & settings):
    m_addParenthesis(addParenthesis),
    m_settings(settings),
    m_name((classname != "" ? classname + "::" : "") + funcname + (m_addParenthesis? "()" : ""))
{
    Logger(m_settings) << log_func << m_name << " {";
    m_settings.beginFunction(m_name);
    m_startTime = std::chrono::high_resolution_clock::now();
}

FuncTracer::FuncTracer(std::string const& funcname, bool addParenthesis, LoggerSettings & settings):
    FuncTracer("", funcname, addParenthesis, settings)
{}

FuncTracer::FuncTracer(bool addParenthesis, LoggerSettings & settings):
    FuncTracer("", "", addParenthesis, settings)
{}

FuncTracer::~FuncTracer()
{
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - m_startTime);
    Logger(m_settings) << log_func << "FUNC_TIME; " << time_span.count() << "; " << m_name;
    m_settings.endFunction();
    Logger(m_settings) << log_func << "} " << m_name;
}

template <>
void FuncArg(std::string const& argname, std::string const& argvalue, LoggerSettings & settings)
{
    Logger(settings) << log_arg << argname << " = '" << argvalue << "'";
}
