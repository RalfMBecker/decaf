/********************************************************************
* tables.h - header file for tables.cpp
*
********************************************************************/

#include <map>
#include <string>

extern std::map<tokenType, int> bin_OpTable;
extern std::map<std::string, std::string> type_Table;

void makeBinOpTable(void);
void makeTypeTable(void);

// (name, basic/class) pairs
class Env{
public:
    Env(Env* P = 0)
	: prior(P) {}

    Env* getPrior() const { return prior; }

    int findName(std::string name)
    {
	if ( (ST.end() == ST.find(name)) )
	    return -1;
	else
	    return 0;
    }

    Env& insertName(std::string name, std::string type)
    {
	if ( (-1 == findName(name)) )
	    ST[name] = type;
	return *this;
    }

    std::string readName(std::string name)
    {
	if ( (-1 == findName(name)) )
	    return "";
	else
	    return ST[name];
    }

private:
    Env* prior;
    std::map<std::string, std::string> ST;
};
