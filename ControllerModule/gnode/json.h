//
//  json.h
//  WebGuiPP
//
//  JSON parser and stringifier implementation.
//
//  Created by Niksa Orlic on 15/08/15.
//  Copyright (c) 2015 Geolux. All rights reserved.
//

#include "../json/json.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <functional>

#include "std_fix.h"

#ifndef WebGuiPP_json_h
#define WebGuiPP_json_h


namespace gnode {
  class js_obj {
  public:
    typedef std::vector<std::shared_ptr<js_obj> > js_obj_vector;
    typedef std::shared_ptr<js_obj_vector> js_obj_vector_ptr;
    
    typedef std::unordered_map<std::string, std::shared_ptr<js_obj> > js_obj_map;
    typedef std::shared_ptr<js_obj_map> js_obj_map_ptr;

    virtual ~js_obj() {}
    
    virtual bool is_null() const { return false; };
    virtual bool is_bool() const { return false; };
    virtual bool is_integer() const { return false; };
    virtual bool is_double() const { return false; };
    virtual bool is_string() const { return false; };
    virtual bool is_array() const { return false; };
    virtual bool is_object() const { return false; };
    
    virtual bool get_bool() const { return false; };
    virtual void set_bool(bool) {  };
    
    virtual int64_t get_integer() const { return 0; };
    virtual void set_integer(int64_t) const { };
    
    virtual double get_double() const { return 0.0; };
    virtual void set_double(double) const { };
    
    virtual std::string get_string() const { return ""; };
    virtual void set_string(const std::string&) {  };
    
    virtual js_obj_vector_ptr get_array() const { return js_obj_vector_ptr(); };
    virtual void set_array(js_obj_vector_ptr) { };
    
    virtual js_obj_map_ptr get_object() const { return js_obj_map_ptr(); };
    virtual void set_object(js_obj_map_ptr) {  };
    
    virtual std::string to_string(int indent = 0, bool same_row = true) const = 0;
    virtual int64_t to_int() const = 0;
    virtual double to_double() const = 0;
    
    virtual std::shared_ptr<js_obj> get(const std::string&) const { return std::shared_ptr<js_obj>(); }
    virtual void set(const std::string&, std::shared_ptr<js_obj>) { }

    virtual std::shared_ptr<js_obj> get(const char*) const { return std::shared_ptr<js_obj>(); }
    virtual void set(const char*, std::shared_ptr<js_obj>) { }
   
    virtual const std::string operator[](const std::string&) const {
      return "";
    }
  };
  
  typedef std::shared_ptr<js_obj> js_obj_ptr;
  
  class js_obj_null : public js_obj {
  public:
    virtual bool is_null() const { return true; }
    
    virtual std::string to_string(int indent = 0, bool same_row = true) const { return std::string(same_row ? 0 : indent, ' ') + "null"; };
    virtual int64_t to_int() const { return 0; }
    virtual double to_double() const { return 0.0; }
  };
  
  class js_obj_bool : public js_obj {
  public:
    js_obj_bool(bool value) : _value(value) {}
    
    virtual bool is_bool() const { return true; };
    
    virtual bool get_bool() const { return _value; };
    virtual void set_bool(bool v) { _value = v; };
    
    virtual std::string to_string(int indent = 0, bool same_row = true) const { return std::string(same_row ? 0 : indent, ' ') + (_value ? "true" : "false"); };
    virtual int64_t to_int() const { return _value ? 1 : 0; }
    virtual double to_double() const { return _value ? 1.0 : 0.0; }
    
  private:
    bool _value;
  };
  
  class js_obj_integer : public js_obj {
  public:
    js_obj_integer(int64_t value) : _value(value) {}
    
    virtual bool is_integer() const { return true; };
    
    virtual std::string get_string() const { return std::to_string(_value); };
    
    virtual int64_t get_integer() const { return _value; };
    virtual void set_integer(int64_t v) { _value = v; };
    
    virtual std::string to_string(int indent = 0, bool same_row = true) const { return std::string(same_row ? 0 : indent, ' ') + std::to_string(_value); };
    virtual int64_t to_int() const { return _value; }
    virtual double to_double() const { return _value; }
    
  private:
    int64_t _value;
  };
  
  class js_obj_double : public js_obj {
  public:
    js_obj_double(double value) : _value(value) {}
    
    virtual bool is_double() const { return true; };
    
    virtual std::string get_string() const { return to_string_digits(_value, 0, 8); };
    
    virtual double get_double() const { return _value; };
    virtual void set_double(double v) { _value = v; };
    
    virtual std::string to_string(int indent = 0, bool same_row = true) const { return std::string(same_row ? 0 : indent, ' ') + to_string_digits(_value, 0, 8); };
    virtual int64_t to_int() const { return _value; }
    virtual double to_double() const { return _value; }
    
  private:
    double _value;
  };
  
  class js_obj_string : public js_obj {
  public:
    js_obj_string(const std::string& value) : _value(value) {}
    
    virtual bool is_string() const { return true; };
    
    virtual std::string get_string() const { return _value; };
    virtual void set_string(const std::string& v) { _value = v; };
    
    virtual std::string to_string(int indent = 0, bool same_row = true) const {
      std::string res = std::string(same_row ? 0 : indent, ' ') + "\"";
      res.reserve(_value.length());
      for(auto i = 0; i < _value.length(); i++) {
        if (_value[i] == '\"') {
          res.push_back('\\');
          res.push_back('\"');
        }
        else if (_value[i] == '\'') {
          res.push_back('\\');
          res.push_back('\'');
        }
        else if (_value[i] == '\\') {
          res.push_back('\\');
          res.push_back('\\');
        }
        else
          res.push_back(_value[i]);
      }
      res.push_back('\"');
      return res;
    };
    
    virtual int64_t to_int() const { return std::stoi(_value); }
    virtual double to_double() const { return std::stod(_value); }
    
  private:
    std::string _value;
  };
  
  class js_obj_array : public js_obj {
  public:
    js_obj_array() {
      _value = js_obj_vector_ptr(new js_obj_vector());
    }
    
    virtual bool is_array() const { return true; };
    
    virtual js_obj_vector_ptr get_array() const { return _value; };
    virtual void set_array(js_obj_vector_ptr v) { _value = v; };
    
    virtual std::string to_string(int indent = 0, bool same_row = true) const {
      std::string res = std::string(same_row ? 0 : indent, ' ');
      res += "[\n";
      
      for(auto i = 0; i < _value->size(); i++) {
        res += _value->at(i)->to_string(indent + 1, false);
        if (i < (_value->size() - 1)) {
          res.push_back(',');
        }
        res.push_back('\n');
      }
      
      res += std::string(indent, ' ');
      res.push_back(']');
      return res;
    }
    
    virtual int64_t to_int() const { return 0; }
    virtual double to_double() const { return 0.0; }
    
  private:
    js_obj_vector_ptr _value;
  };
  
  class js_obj_object : public js_obj {
  public:
    js_obj_object() {
      _value = js_obj_map_ptr(new js_obj_map());
    }
    
    virtual bool is_object() const { return true; };
    
    virtual js_obj_map_ptr get_object() const { return _value; };
    virtual void set_object(js_obj_map_ptr v) { _value = v; };
    
    virtual std::string to_string(int indent = 0, bool same_row = true) const {
      std::string res = std::string(same_row ? 0 : indent, ' ') + "{";
      
      bool first = true;
      for(auto it = _value->begin(); it != _value->end(); ++it) {
        if (first)
          first = false;
        else
          res.push_back(',');
        
        res.push_back('\n');
        res += std::string(indent + 1, ' ');
        res.push_back('\"');
        res += it->first;
        res.push_back('\"');
        res.push_back(':');
        res += it->second->to_string(indent + 2, true);
      }
      
      res.push_back('\n');
      res += std::string(indent, ' ');
      res.push_back('}');
      return res;
    }

    std::string to_string(const std::string& skip_field) {
      std::string res = "{";

      bool first = true;
      for(auto it = _value->begin(); it != _value->end(); ++it) {
        if (it->first == skip_field)
          continue;
        if (first)
          first = false;
        else
          res.push_back(',');

        res.push_back('\n');
        res += std::string(1, ' ');
        res.push_back('\"');
        res += it->first;
        res.push_back('\"');
        res.push_back(':');
        res += it->second->to_string(2, true);
      }

      res.push_back('\n');
      res.push_back('}');
      return res;
    }
    
    virtual int64_t to_int() const { return 0; }
    virtual double to_double() const { return 0.0; }
    
    virtual js_obj_ptr get(const std::string& key) const {
      if (_value->find(key) == _value->end())
        return js_obj_ptr();
      
      return _value->at(key);
    }

    virtual js_obj_ptr get(const char* key) const {
      if (_value->find(key) == _value->end())
        return js_obj_ptr();
      
      return _value->at(key);
    }
    
    virtual void set(const std::string& key, std::shared_ptr<js_obj> obj) {
      if (_value->find(key) != _value->end())
        _value->at(key) = obj;
      else
        _value->insert({{key, obj}});
    }

    virtual void set(const char* key, std::shared_ptr<js_obj> obj) {
      if (_value->find(key) != _value->end())
        _value->at(key) = obj;
      else
        _value->insert({{key, obj}});
    }

    virtual const std::string operator[](const std::string& key) const {
      auto it = get(key);
      if (!it)
        return "";
      return it->get_string();
    }


  private:
    js_obj_map_ptr _value;
  };
  
  class jsonizable {
    struct property_desc {
      const char* name;
      std::function<void(std::string&)> func;
      
      property_desc(const char* name, std::function<void(std::string&)> func) : name(name), func(func) {}
    };
    typedef std::shared_ptr<property_desc> property_desc_ptr;
    
    static property_desc_ptr make_property_desc(const char* name, std::function<void(std::string&)> func) {
      return property_desc_ptr(new property_desc(name, func));
    }
    
  public:
    void set_properties(std::function<void(void)> f) {
      _set_properties = f;
    }
    
    template <class T>
    void property(const char* name, T* v) {
      _jsonizers.push_back(make_property_desc(name, [v](std::string& s){ v->to_string(s); }));
    }
    
    void property(const char* name, const float* v) {
      _jsonizers.push_back(make_property_desc(name, [v](std::string& s){ s += to_string_digits(*v, 0, 2); }));
    }
    
    void property(const char* name, gnode::js_obj_ptr obj, const char* exclude = nullptr) {
      _jsonizers.push_back(make_property_desc(name, [obj, exclude](std::string& s){ s += reinterpret_cast<js_obj_object*>(obj.get())->to_string(exclude); }));
    }
    
    void property(const char* name, int* v) {
      _jsonizers.push_back(make_property_desc(name, [v](std::string& s){ s.append(std::to_string(*v)); }));
    }
    
    void property(const char* name, unsigned int* v) {
      _jsonizers.push_back(make_property_desc(name, [v](std::string& s){ s.append(std::to_string(*v)); }));
    }
    
    void property(const char* name) {
      _jsonizers.push_back(make_property_desc(name, [](std::string& s){ s.append("null"); }));
    }
    
    template <class T>
    void property(const char* name, std::vector<T>* v) {
      _jsonizers.push_back(make_property_desc(name, [v](std::string& s){
        s.push_back('[');
        for(auto i = 0; i < v->size(); i++) {
          if (i > 0)
            s.push_back(',');
          s += v->at(i)->to_string();
        }
        s.push_back(']');
      }));
    }
    
    void property(const char* name, std::deque<int>* v) {
      _jsonizers.push_back(make_property_desc(name, [v](std::string& s){
        s.push_back('[');
        for(auto i = 0; i < v->size(); i++) {
          if (i > 0)
            s.push_back(',');
          s += std::to_string(v->at(i));
        }
        s.push_back(']');
      }));
    }
    
    template <class T1, class T2>
    void property(const char* name, std::deque<std::deque<T1, T2>>* v) {
      _jsonizers.push_back(make_property_desc(name, [v](std::string& s){
        s.push_back('[');
        for(auto i = 0; i < v->size(); i++) {
          if (i > 0)
            s.push_back(',');
          
          s.push_back('{');
          for(auto j = 0; j < v->at(i).size(); j++) {
            if (j > 0)
              s.push_back(',');
            
            s += "\"";
            s += std::to_string(v->at(i).at(j).first);
            s += "\": ";
            s += std::to_string(v->at(i).at(j).second);
          }
          s.push_back('}');
          
        }
        s.push_back(']');
      }));
    }
    
    void property(const char* name, std::string* v) {
      _jsonizers.push_back(make_property_desc(name, [v](std::string& s){
        s.push_back('"');
        for(auto i = 0; i < v->length(); i++) {
          switch(v->at(i)) {
            case '"':
              s.push_back('\\');
              s.push_back('\"');
              break;
            case '\'':
              s.push_back('\\');
              s.push_back('\'');
              break;
            case '\\':
              s.push_back('\\');
              s.push_back('\\');
              break;
            default:
              s.push_back(v->at(i));
          }
        }
        s.push_back('"');
      }));
    }
    
    void property(const char* name, const char* v) {
      _jsonizers.push_back(make_property_desc(name, [v](std::string& s){
        s.push_back('"');
        auto len = strlen(v);
        for(auto i = 0; i < len; i++) {
          switch(v[i]) {
            case '"':
              s.push_back('\\');
              s.push_back('\"');
              break;
            case '\'':
              s.push_back('\\');
              s.push_back('\'');
              break;
            case '\\':
              s.push_back('\\');
              s.push_back('\\');
              break;
            default:
              s.push_back(v[i]);
          }
        }
        s.push_back('"');
      }));
    }
    
    void property(const char* name, float* v) {
      _jsonizers.push_back(make_property_desc(name, [v, name](std::string& s){
        s.append(std::to_string(*v)); }));
    }
    
    void property(const char* name, double* v) {
      _jsonizers.push_back(make_property_desc(name, [v, name](std::string& s){
        s.append(std::to_string(*v)); }));
    }
    
    void property(const char* name, bool* v) {
      _jsonizers.push_back(make_property_desc(name, [v](std::string& s){ s.append(*v ? "true" : "false"); }));
    }
    
    std::string to_string() const {
      std::string s;
      to_string(s);
      return s;
    }
    
    void to_string(std::string& s) const {
      if (_jsonizers.empty()) {
        _set_properties();
      }
     
      s.push_back('{');
      for(auto i = 0; i < _jsonizers.size(); i++) {
        if (i > 0)
          s.push_back(',');
        s.push_back('\"');
        s.append(_jsonizers[i]->name);
        s.append("\":");
        _jsonizers[i]->func(s);
      }
      s.push_back('}');
    }
    
  private:
    std::function<void(void)> _set_properties;
    std::vector<property_desc_ptr> _jsonizers;
  };
  
  
  class json {
  public:
    static js_obj_ptr parse(const std::string& s) {
      Json::Reader reader;
      Json::Value root;
      if (!reader.parse(s, root, false))
        return js_obj_ptr();
      
      return reader_to_json(root);
    }
    
    static std::string stringify(js_obj_ptr o) {
      return o->to_string();
    }

   static std::string stringify(js_obj_ptr o, const std::string& skip_field) {
     if (o->is_object())
       return static_cast<js_obj_object*>(o.get())->to_string(skip_field);
     else
       return o->to_string();
   }
    
  private:
    static js_obj_ptr reader_to_json(Json::Value& v) {
      if (v.isNull())
        return js_obj_ptr(new js_obj_null());
      
      if (v.isBool())
        return js_obj_ptr(new js_obj_bool(v.asBool()));
      
      if (v.isIntegral())
        return js_obj_ptr(new js_obj_integer(v.asInt64()));
      
      if (v.isNumeric())
        return js_obj_ptr(new js_obj_double(v.asDouble()));
      
      if (v.isString())
        return js_obj_ptr(new js_obj_string(v.asString()));
      
      if (v.isArray()) {
        js_obj_ptr rv = js_obj_ptr(new js_obj_array());
        
        for(auto i = 0; i < v.size(); i++) {
          rv->get_array()->push_back(reader_to_json(v[i]));
        }
        
        return rv;
      }
      
      if (v.isObject()) {
        js_obj_ptr rv = js_obj_ptr(new js_obj_object());
        
        for(auto i = v.begin(); i != v.end(); ++i) {
          rv->set(i.memberName(), reader_to_json(*i));
        }
        
        return rv;
      }
      
      return js_obj_ptr();
    }
  };

  inline js_obj_ptr make_js_obj_ptr(const std::string& s) {
    return js_obj_ptr(new js_obj_string(s));
  }
  
  inline js_obj_ptr make_js_obj_ptr(int v) {
    return js_obj_ptr(new js_obj_integer(v));
  }
  
  inline js_obj_ptr make_js_obj_ptr(double v) {
    return js_obj_ptr(new js_obj_double(v));
  }
  
  inline js_obj_ptr make_js_obj_ptr(js_obj_ptr v) {
    return v;
  }
  
  template <class T>
  inline js_obj_ptr make_js_obj_ptr(const std::vector<T>& v) {
    auto res = new js_obj_array();
    auto vec = js_obj::js_obj_vector_ptr(new js_obj::js_obj_vector());
    res->set_array(vec);
    
    for(auto i = 0; i < v.size(); i++) {
      vec->push_back(make_js_obj_ptr(v[i]));
    }
    
    return js_obj_ptr(res);
  }
  
  template <class T>
  inline js_obj_ptr make_js_obj_ptr(const std::deque<T>& v) {
    auto res = new js_obj_array();
    auto vec = js_obj::js_obj_vector_ptr(new js_obj::js_obj_vector());
    res->set_array(vec);
    
    for(auto i = 0; i < v.size(); i++) {
      vec->push_back(make_js_obj_ptr(v[i]));
    }
    
    return js_obj_ptr(res);
  }
  

}; // end namespace gnode


#endif
