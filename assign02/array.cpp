#include "array.h"
#include "exceptions.h"
#include "valrep.h"

Array::Array(std::vector<Value> arr) 
  : ValRep(VALREP_ARRAY)
  , arr(arr) {

}

Array::~Array() {

}

void Array::set_val(Value val, int ind) {
  arr[ind] = val;
}