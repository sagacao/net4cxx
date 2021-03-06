//
// Created by yuwenyong on 17-9-19.
//

#ifndef NET4CXX_COMMON_HTTPUTILS_COOKIE_H
#define NET4CXX_COMMON_HTTPUTILS_COOKIE_H

#include "net4cxx/common/common.h"
#include "net4cxx/common/utilities/errors.h"

NS_BEGIN

NET4CXX_DECLARE_EXCEPTION(CookieError, Exception);


class NET4CXX_COMMON_API CookieUtil {
public:
    static std::string quote(const std::string &s, const std::vector<char> &legalChars= CookieUtil::LegalChars,
                             const std::array<char, 256> &idmap= CookieUtil::Idmap);

    static std::string unquote(const std::string &s);

    static const std::vector<char> LegalChars;
    static const std::array<char, 256> Idmap;
    static const std::map<char, std::string> Translator;
    static const boost::regex OctalPatt;
    static const boost::regex QuotePatt;
    static const char *LegalCharsPatt;
    static const boost::regex CookiePattern;
};


class NET4CXX_COMMON_API Morsel {
public:
    Morsel();

    std::string& operator[](const std::string &key);

    bool isReservedKey(const std::string &key) const {
        return reserved.find(boost::to_lower_copy(key)) != reserved.end();
    }

    const std::string& getValue() const {
        return _value;
    }

    void set(const std::string &key, const std::string &val, const std::string &codedVal,
             const std::vector<char> &legalChars= CookieUtil::LegalChars,
             const std::array<char, 256> &idmap= CookieUtil::Idmap);

    std::string output(const StringMap *attrs= nullptr, const std::string &header= "Set-Cookie:") const {
        return header + " " + outputString(attrs);
    }

    std::string outputString(const StringMap *attrs= nullptr) const;

    static const StringMap reserved;
protected:
    StringMap _items;
    std::string _key;
    std::string _value;
    std::string _codedValue;
};


class NET4CXX_COMMON_API BaseCookie {
public:
    typedef std::tuple<std::string, std::string> DecodeResultType;
    typedef std::tuple<std::string, std::string> EncodeResultType;
    typedef std::map<std::string, Morsel> MorselContainerType;
    typedef std::function<void (const std::string&, const Morsel&)> CallbackType;

    class CookieSetter {
    public:
        CookieSetter(BaseCookie *cookie, const std::string &key)
                : _cookie(cookie)
                , _key(key) {

        }

        CookieSetter& operator=(const std::string &value) {
            std::string rval, cval;
            std::tie(rval, cval) = _cookie->valueEncode(value);
            _cookie->set(_key, rval, cval);
            return *this;
        }
    protected:
        BaseCookie *_cookie;
        std::string _key;
    };

    BaseCookie() = default;

    explicit BaseCookie(const std::string &input) {
        load(input);
    }

    explicit BaseCookie(const StringMap &input) {
        load(input);
    }

    virtual ~BaseCookie() = default;

    const Morsel& at(const std::string &key) const {
        return _items.at(key);
    }

    Morsel& at(const std::string &key) {
        return _items.at(key);
    }

    CookieSetter operator[](const std::string &key) {
        return CookieSetter{this, key};
    }

    bool has(const std::string &key) const {
        return _items.find(key) != _items.end();
    }

    void erase(const std::string &key) {
        _items.erase(key);
    }

    void getAll(const CallbackType &callback) const {
        for (auto &item: _items) {
            callback(item.first, item.second);
        }
    }

    virtual DecodeResultType valueDecode(const std::string &val);

    virtual EncodeResultType valueEncode(const std::string &val);

    std::string output(const StringMap *attrs= nullptr, const std::string &header= "Set-Cookie:",
                       const std::string &sep= "\015\012") const;

    void load(const std::string &rawdata) {
        parseString(rawdata);
    }

    void load(const StringMap &rawdata) {
        for (auto &kv: rawdata) {
            (*this)[kv.first] = kv.second;
        }
    }

    void clear() {
        _items.clear();
    }
protected:
    void set(const std::string &key, const std::string &realValue, const std::string &codedValue) {
        auto iter = _items.find(key);
        if (iter == _items.end()) {
            Morsel m;
            m.set(key, realValue, codedValue);
            _items.emplace(key, std::move(m));
        } else {
            iter->second.set(key, realValue, codedValue);
        }
    }

    void parseString(const std::string &str, const boost::regex &patt =CookieUtil::CookiePattern);

    MorselContainerType _items;
};


class NET4CXX_COMMON_API SimpleCookie: public BaseCookie {
public:
    using BaseCookie::BaseCookie;

    SimpleCookie() = default;

    DecodeResultType valueDecode(const std::string &val) override;

    EncodeResultType valueEncode(const std::string &val) override;
};

NS_END

#endif //NET4CXX_COMMON_HTTPUTILS_COOKIE_H
