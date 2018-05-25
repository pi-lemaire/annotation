#ifndef ParamsHandler_H
#define ParamsHandler_H


#include <iostream>
#include <vector>
#include <map>
#include <typeinfo>
#include <memory> // For std::unique_ptr




// define readable typenames
// they will be used for the interface, to know with which data type it is playing

// by default, types are defined by their typeid(T).name() function
// however, typeid(T).name() is quite compiler-dependant, and/or a hassle to read
// so we define a macro as a specialization that allows us to read the exact same type as we code it

// the class TypeParseTraits is exclusively static
class TypeParseTraits
{
public:
    template<typename T>
    static const std::string& getTypeName() { static std::string _ret = typeid(T).name(); return _ret; }
        // the string is stored in static, so that we can provide the reference only
};

// the macro to specialize it is the following - don't forget the inline - if not, this header is impossible to include without linker error
#define REGISTER_PARSE_TYPE(X) template <> inline const std::string& TypeParseTraits::getTypeName<X>() { static std::string _ret = #X ; return _ret; }

// record some useful types...
REGISTER_PARSE_TYPE(int)
REGISTER_PARSE_TYPE(double)
REGISTER_PARSE_TYPE(float)
REGISTER_PARSE_TYPE(std::string)







// now define what a parameter is and what we can do with it
// a parameter has a name, a description and sometimes even specificities (boundaries, priority, etc) that we will encode using a string
// it is also related to a value on which we can perform get and set

// first, we have to pre-declare the template instances
template <typename T> class ParamClass;
class ParamsHandler;

// then the declaration in itself
class ParamInterface
{
public:
    ParamInterface(ParamsHandler* pHPointer, const std::string& vName, const std::string& vDesc="", const std::string& vSpecs="") : varName(vName), varDescription(vDesc), varSpecificities(vSpecs) { this->parentPointer = pHPointer; }
    virtual ~ParamInterface() {}

    // get the standard, textual informations
    const std::string& getName() const { return this->varName; }
    const std::string& getDescription() const { return this->varDescription; }
    const std::string& getSpecificities() const { return this->varSpecificities; }

    virtual const std::string& getType() const = 0;

    // we also declare the standard accessors get and set as templates
    // the use of static_casts ensures that we're not going to call mistakingly a get<int>() over a param<float> object
    // i'm not sure whether this will be useable once multiple heritage has been used...?
    // if so, we switch to dynamic_cast
    template<typename T>
    const T& getValue() const
    {
        return static_cast<ParamClass<T> const&>(*this).getValue();
    }

    // had to only declare this one here, then write its content later, because of the use of a method from ParamInterface
    template<typename T>
    void setValue(const T& val);
    /*
    {
        static_cast<ParamClass<T> &>(*this).setValue(val);

        this->parentPointer->setValueCalled();
    }
    */



private:
    std::string varName, varDescription, varSpecificities;
    ParamsHandler *parentPointer;
};




// template instances now
// the idea is to have all of the above + the get and set things, and foremost the pointer to the value with which we want to play with
// this implementation is pretty straightforward actually

template <typename T>
class ParamClass : public ParamInterface
{
public:
    ParamClass(ParamsHandler* pHPointer, const std::string& vName, T* addr, const std::string& vDesc="", const std::string& vSpecs="")
        : ParamInterface(pHPointer, vName, vDesc, vSpecs)
    {
        this->valueAddress = addr;
    }
    virtual ~ParamClass() { /*std::cout << "delete called for type " << this->getType() << std::endl;*/ }
    // virtual const std::string& getType() const { return TypeParseTraits<T>::name; }
    //virtual const std::string& getType() const { return typeid(T).name(); }
    virtual const std::string& getType() const { return TypeParseTraits::getTypeName<T>(); }

    const T& getValue() const { return *(this->valueAddress); }
    void setValue(const T& val) { *(this->valueAddress) = val; }

private:
    T* valueAddress;
};





// now the class that handles everything
// it is pretty simple, it embeds a vector as well as a map for

class ParamsHandler
{
public:
    ParamsHandler() {}
    ~ParamsHandler() {}

    // get the value for a given index (the index can be found through the help of the searchIndex() function)
    template<typename T>
    const T& getValue(size_t index) const
    {
        return this->paramsVec[index]->getValue<T>();
    }

    // set the value at a given index, same commentary as above
    template<typename T>
    void setValue(size_t index, const T& val)
    {
        this->paramsVec[index]->setValue<T>(val);

        // this->setValueCalled();
    }

    // lets us access to the parameters info
    const std::unique_ptr<ParamInterface>& getParamInfos(size_t index) const
    {
        return this->paramsVec[index];
    }

    // complete accessor because it is more convenient to use when building the interface..
    ParamInterface* accessLine(size_t index)
    {
        return this->paramsVec[index].get();
    }

    // size of the vector, so that we know how many parameters there are
    size_t getParamsNumber() const
    {
        return this->paramsVec.size();
    }

    // search index using the var name as a key - not necessarily too useful, but just in case...
    size_t searchIndex(const std::string& key) const
    {
        auto search = this->nameIndices.find(key);
        if (search != this->nameIndices.end())
            return search->second;

        // default, should not happen if the function is called properly
        return 0;
    }

    const std::string& getParametersSectionName() const { return this->parametersSectionName; }

    virtual void setValueCalled(const std::string& paramName) { if (paramName.length()==0) std::cout << "setValueCalled bug" << std::endl; }    // i put something useless to avoid the warnings



protected:
    // the procedure in which we specify which parameters and how we use them
    // we set it as virtual, we want to force the user to complete it when he builds a class that inheritates ParamsHandler
    virtual void initParamsHandler() =0;

    // pushParams is the function that should be solely used inside initParamsHandler
    template<typename T>
    void pushParam(const std::string& vName, T* addr, const std::string& vDesc="", const std::string& vSpecs="")
    {
        size_t currentIndex = this->paramsVec.size();
        this->paramsVec.emplace_back( new ParamClass<T>(this, vName, addr, vDesc, vSpecs) );
        this->nameIndices.insert(std::make_pair(vName, currentIndex));
    }


    std::string parametersSectionName;

private:
    std::vector<std::unique_ptr<ParamInterface>> paramsVec;
    std::map<std::string, size_t> nameIndices;
};



// we had to declare it after the paramHandler declaration in order to get the "setValueCalled" thing working...
// quite annoying but anyway, I believe that the setValue is potentially quite useful...
template<typename T>
void ParamInterface::setValue(const T& val)
{
    static_cast<ParamClass<T> &>(*this).setValue(val);

    this->parentPointer->setValueCalled(this->varName);
}





// just an example class to prove that this thing works

class ParamsHandlerInst : public ParamsHandler
{
public:
    ParamsHandlerInst() { this->initParamsHandler(); this->value=3; this->stringValue = "hophophop"; this->valFloat = 0.4; this->valDbl = 1e-5; }
    ~ParamsHandlerInst() {}

    void setValueCalled(const std::string& paramName) { this->stringValue.append(paramName); }

protected:
    virtual void initParamsHandler()
    {
        this->parametersSectionName = "Params Section";
        this->pushParam<int>("var1", &(this->value), "useless description");
        this->pushParam<int>("var1 again", &(this->value), "yet another useless description");
        this->pushParam<std::string>("var3", &(this->stringValue), "this is a string description");
        this->pushParam<float>("varFloat", &(this->valFloat));
        this->pushParam<double>("varDbl", &(this->valDbl));
    }


private:
    int value;
    float valFloat;
    double valDbl;
    std::string stringValue;
};














#endif // ParamsHandler_H
