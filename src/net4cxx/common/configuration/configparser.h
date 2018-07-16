//
// Created by yuwenyong on 17-9-18.
//

#ifndef NET4CXX_COMMON_CONFIGURATION_CONFIGPARSER_H
#define NET4CXX_COMMON_CONFIGURATION_CONFIGPARSER_H

#include "net4cxx/common/common.h"
#include <boost/property_tree/ptree.hpp>
#include "net4cxx/common/utilities/errors.h"


NS_BEGIN

NET4CXX_DECLARE_EXCEPTION(NoSectionError, Exception);
NET4CXX_DECLARE_EXCEPTION(NoOptionError, Exception);


class NET4CXX_COMMON_API ConfigParser {
public:
    typedef boost::property_tree::ptree ConfigType;

    void read(const std::string &fileName);

    void write(const std::string &fileName);

    StringVector getSections() const;

    bool hasSection(const std::string &section) const {
        return _config.find(section) != _config.not_found();
    }

    bool removeSection(const std::string &section);

    StringVector getOptions(const std::string &section, bool ignoreError=true) const;

    bool hasOption(const std::string &section, const std::string &option) const;

    bool removeOption(const std::string &section, const std::string &option);

    StringMap getItems(const std::string &section, bool ignoreError=true) const;

    std::string getString(const std::string &section, const std::string &name) const {
        std::string retVal = get<std::string>(section, name);
        retVal.erase(std::remove(retVal.begin(), retVal.end(), '"'), retVal.end());
        return retVal;
    }

    std::string getString(const std::string &section, const std::string &name, const std::string &defaultValue) const {
        std::string retVal = get(section, name, defaultValue);
        retVal.erase(std::remove(retVal.begin(), retVal.end(), '"'), retVal.end());
        return retVal;
    }

    bool getBoolean(const std::string &section, const std::string &name) const {
        std::string retVal = get<std::string>(section, name);
        retVal.erase(std::remove(retVal.begin(), retVal.end(), '"'), retVal.end());
        return (retVal == "1" || retVal == "true" || retVal == "TRUE" || retVal == "yes" || retVal == "YES");
    }

    bool getBoolean(const std::string &section, const std::string &name, bool defaultValue) const {
        std::string retVal = get(section, name, std::string(defaultValue ? "1" : "0"));
        retVal.erase(std::remove(retVal.begin(), retVal.end(), '"'), retVal.end());
        return (retVal == "1" || retVal == "true" || retVal == "TRUE" || retVal == "yes" || retVal == "YES");
    }

    int getInt(const std::string &section, const std::string &name) const {
        return get<int>(section, name);
    }

    int getInt(const std::string &section, const std::string &name, int defaultValue) const {
        return get(section, name, defaultValue);
    }

    float getFloat(const std::string &section, const std::string &name) const {
        return get<float>(section, name);
    }

    float getFloat(const std::string &section, const std::string &name, float defaultValue) const {
        return get(section, name, defaultValue);
    }

    void setString(const std::string &section, const std::string &option, const std::string &value) {
        set(section, option, value);
    }

    void setString(const std::string &section, const std::string &option, std::string &&value) {
        set(section, option, std::move(value));
    }

    void setBoolen(const std::string &section, const std::string &option, bool value) {
        set(section, option, value ? "true" : "false");
    }

    void setInt(const std::string &section, const std::string &option, int value) {
        set(section, option, value);
    }

    void setFloat(const std::string &section, const std::string &option, float value) {
        set(section, option, value);
    }
protected:
    template <typename ValueT>
    ValueT get(const std::string &section, const std::string &option) const {
        if (!hasSection(section)) {
            NET4CXX_THROW_EXCEPTION(NoSectionError, "No section: %s", section);
        }
        if (!hasOption(section, option)) {
            NET4CXX_THROW_EXCEPTION(NoOptionError, "No option %s in section: %s", option, section);
        }
        ValueT retVal;
        try {
            retVal = _config.get<ValueT>(section + '.' + option);
        } catch (boost::property_tree::ptree_bad_path &) {
            NET4CXX_THROW_EXCEPTION(KeyError, "Missing name %s.%s", section, option);
        } catch (boost::property_tree::ptree_bad_data &) {
            NET4CXX_THROW_EXCEPTION(TypeError, "Bad value for name %s.%s", section, option);
        }
        return retVal;
    }

    template <typename ValueT>
    ValueT get(const std::string &section, const std::string &option, const ValueT &defaultValue) const {
        return _config.get(section + '.' + option, defaultValue);
    }

    template <typename ValueT>
    void set(const std::string &section, const std::string &option, ValueT &&value) {
        _config.put(section + '.' + option, std::forward<ValueT>(value));
    }

    ConfigType _config;
};

NS_END

#endif //NET4CXX_COMMON_CONFIGURATION_CONFIGPARSER_H
