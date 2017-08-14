#ifndef Group20170807H
#define Group20170807H

#include <string>

namespace goby
{
    class Group
    {
    public:
        constexpr Group(const char* c = "") : c_(c) { }
        constexpr Group(int i) : i_(i) { }
        
        operator std::string() const
        {   
            if(c_ != nullptr) return std::string(c_);
            else return std::to_string(i_);
        }
        
        constexpr operator int() const { return i_; }   
        constexpr const char* c_str() const { return c_; }
        
    private:
        int i_{0};
        const char* c_{nullptr};
    };
    
    inline bool operator==(const Group& a, const Group& b)
    { return std::string(a) == std::string(b); }
    
    inline bool operator<(const Group& a, const Group& b)
    { return std::string(a) < std::string(b); }
    
    
    template<const Group& group>
        void check_validity()
    {
        static_assert((int(group) != 0) || (group.c_str()[0] != '\0'), "goby::Group must have non-zero length string or non-zero integer value.");
    }

    inline void check_validity_runtime(const Group& group)
    {
        // currently no-op - InterVehicleTransporterBase allows empty groups
        // TODO - check if there's anything we can check for here
    }
}


#endif
