/********************************************************************
* tables.h - header file for tables.cpp
*
********************************************************************/

#include <map>
#include <string>

// compile type globals
extern std::map<tokenType, int> bin_OpTable;
extern std::map<std::string, std::string> type_Table;
extern std::map<std::string, int> type_WidthTable;
class Env;
extern Env* root_Env;

// runtime global
class symbolTable;
extern std::map<std::string, symbolTable> STs;

void makeBinOpTable(void);
void makeTypePrecTable(void);
int typePriority(std::string); // relocate as method of IDs & Arrays
int typeWidth(std::string); // relocate as method of IDs & Arrays & Classes

// Compile-time object, part of a linked list of (ct) Symbol Tables, each
// a (name, <basic type>/class) pair
// Handling 'name' here isn't the most logical, but will do (name=table name).
// Name is needed to maintain the run-time version of the STs.
class Env{
public:
    Env(Env* P = 0)
	: prior_(P) { count_++; name_ = "Env"; name_ += count_; }

    Env* getPrior() const { return prior_; }
    std::string getName() const { return name_; }

    int findName(std::string entry_Name)
    {
	if ( (type_.end() == type_.find(entry_Name)) )
	    return -1;
	else
	    return 0;
    }

    Env& insertName(std::string new_Name, std::string t)
    {
	if ( (-1 == findName(new_Name)) )
	    type_[new_Name] = t;
	return *this;
    }

    std::string readName(std::string search_Name)
    {
	if ( (-1 == findName(search_Name)) )
	    return "";
	else
	    return type_[search_Name];
    }

private:
    static int count_;
    std::string name_;
    Env* prior_;
    std::map<std::string, std::string> type_; // <basic type>/class    
};

Env* makeEnvRoot(void);

// Run-time symbol table information
class envInfo{
public:
    envInfo(std::string Type = "", std::string memT = "", int Offset = 0, 
	    int Width = 0)
	: type_(Type), memType_(memT), offset_(Offset), width_(Width) {}

    std::string Type(void) const { return type_; }
    std::string MemType(void) const { return memType_; }
    int Offset(void) const { return offset_; }
    int Width(void) const { return width_; }

private:
    std::string type_; // basic; class
    std::string memType_; // stack; heap
    int offset_; // rel. offset to mem areas reserved by Env
    int width_; // size of object
};


// Run-time object to manage storage allocation
// Name created by the corresponding compile-time object, and fed
// from builder function ************ENTER NAME***************
// Offset: rel offset to beginning of mem area that will be reserved 
//         for objects in the scope managed by this symbolTable, by
//         heap and stack area
class symbolTable{
public:
    symbolTable(std::string Name)
	: name_(Name) {}
    
    // consider making next 3 private
    // manipulating envInfo
    int findName(std::string search_Name)
    {
	if ( (info_.end() == info_.find(search_Name)) )
	    return -1;
	else
	    return 0;
    }

    // Note: method overloaded
    symbolTable& insertName(std::string new_Name, envInfo i)
    {
	if ( (-1 == findName(new_Name)) )
	    info_[new_Name] = i;
	return *this;
    }

    symbolTable& insertName(std::string new_Name, std::string Type, 
			    std::string Mem, int Offset, int Width)
    {
	int tmp;
	if ( ("heap" == Mem) ){
	    offsetHeap_ += Width;
	    tmp = offsetHeap_;
	}
	else if ( ("stack" == Mem) ){
	    offsetStack_ += Width;
	    tmp = offsetStack_;
	}
	else
	    ;
	    // little point singling out this as the only validity check
	    // as function only used by compiler write
	envInfo tmpInfo(Type, Mem, tmp, Width);
	insertName(new_Name, tmpInfo);
	return *this;
    }

    envInfo readNameInfo(std::string read_Name)
    {
	if ( (-1 == findName(read_Name)) )
	    return envInfo(0);
	else
	    return info_[read_Name];
    }

    // retrieving envInfo fields ("" == 'not defined')
    std::string const getType(std::string read_Name)
    {
	envInfo tmp(readNameInfo(read_Name));
	return tmp.Type();
    }

    std::string const getMemType(std::string read_Name)
    {
	envInfo tmp(readNameInfo(read_Name));
	return tmp.MemType();
    }

    int const getOffset(std::string read_Name)
    {
	envInfo tmp(readNameInfo(read_Name));
	return tmp.Offset();
    }

    int const getWidth(std::string read_Name)
    {
	envInfo tmp(readNameInfo(read_Name));
	return tmp.Width();
    }


private:
    static int offsetHeap_;
    static int offsetStack_;
    std::string name_;
    std::map<std::string, envInfo> info_;    
};

