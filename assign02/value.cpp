#include "cpputil.h"
#include "exceptions.h"
#include "valrep.h"
#include "function.h"
#include "value.h"
#include "string.h"
#include "array.h"

Value::Value(int ival)
  : m_kind(VALUE_INT) {
  m_atomic.ival = ival;
}

Value::Value(Function *fn)
  : m_kind(VALUE_FUNCTION)
  , m_rep(fn) {
  m_rep = fn;
  fn->add_ref();
}

Value::Value(IntrinsicFn intrinsic_fn)
  : m_kind(VALUE_INTRINSIC_FN) {
  m_atomic.intrinsic_fn = intrinsic_fn;
}

Value::Value(String* string)
  : m_kind(VALUE_STRING)
  , m_rep(string) {
    m_rep->add_ref();
  }

Value::Value(Array* arr)
  : m_kind(VALUE_ARRAY)
  , m_rep(arr) {
    m_rep->add_ref();
  }

Value::Value(const Value &other)
  : m_kind(VALUE_INT) {
  // Just use the assignment operator to copy the other Value's data
  *this = other;
}

Value::~Value() {
  // handle reference counting (detach from ValRep, if any)

  if (m_kind >= VALUE_FUNCTION) {
    m_rep->remove_ref();
    if (m_rep->get_num_refs() == 0) {
      delete m_rep;
    }
  }
}

Value &Value::operator=(const Value &rhs) {
  if (this != &rhs &&
      !(is_dynamic() && rhs.is_dynamic() && m_rep == rhs.m_rep)) {
    // handle reference counting (detach from previous ValRep, if any)
    if (m_kind >= VALUE_FUNCTION) {
      m_rep->remove_ref();
      if (m_rep->get_num_refs() == 0) {
        delete m_rep;
      }
    }
    m_kind = rhs.m_kind;
    if (is_dynamic()) {
      // attach to rhs's dynamic representation
      m_rep = rhs.m_rep;
      // handle reference counting (attach to the new ValRep)
      m_rep->add_ref();
    } else {
      // copy rhs's atomic representation
      m_atomic = rhs.m_atomic;
    }
  }
  return *this;
}

Function *Value::get_function() const {
  assert(m_kind == VALUE_FUNCTION);
  return m_rep->as_function();
}

Array *Value::get_array() const {
  assert(m_kind == VALUE_ARRAY);
  return m_rep->as_array();
}
String *Value::get_string() const {
  assert(m_kind == VALUE_STRING);
  return m_rep->as_string();
}

std::string Value::as_str() const {
  switch (m_kind) {
  case VALUE_INT:
    return cpputil::format("%d", m_atomic.ival);
  case VALUE_FUNCTION:
    return cpputil::format("<function %s>", m_rep->as_function()->get_name().c_str());
  case VALUE_INTRINSIC_FN:
    return "<intrinsic function>";
  case VALUE_STRING:
    return cpputil::format("%s", m_rep->as_string()->get_text().c_str());
  case VALUE_ARRAY: {
    std::string cur = "[";

    auto arr = m_rep->as_array();

    auto arr_vector=arr->get_array();

    for (auto i = arr_vector.cbegin(); i != arr_vector.cend(); ++i) {
      cur += (*i).as_str();
      if ( (i + 1) != arr_vector.cend() ) {
        cur += ", ";
      }
    }

    cur +="]";

    return cpputil::format("%s", cur.c_str());    
  }
  default:
    // this should not happen
    RuntimeError::raise("Unknown value type %d", int(m_kind));
  }
}

// TODO: implementations of additional member functions
