%module hammer

%nodefaultctor;

%include "stdint.i"

#if defined(SWIGPYTHON)
%ignore HCountedArray_;
%apply (char *STRING, size_t LENGTH) {(uint8_t* str, size_t len)}
%apply (uint8_t* str, size_t len) {(const uint8_t* input, size_t length)}
%apply (uint8_t* str, size_t len) {(const uint8_t* str, const size_t len)}
%apply (uint8_t* str, size_t len) {(const uint8_t* charset, size_t length)}


%rename("_%s") "";
// %rename(_h_ch) h_ch;

%inline {
  static PyObject *_helper_Placeholder = NULL, *_helper_ParseError = NULL;

  static void register_helpers(PyObject* parse_error, PyObject *placeholder) {
    _helper_ParseError = parse_error;
    _helper_Placeholder = placeholder;
  }
 }

%pythoncode %{
  class Placeholder(object):
      """The python equivalent of TT_NONE"""
      def __str__(self):
          return "Placeholder"
      def __repr__(self):
          return "Placeholder"
      def __eq__(self, other):
          return type(self) == type(other)
  class ParseError(Exception):
      """The parse failed; the message may have more information"""
      pass

  _hammer._register_helpers(ParseError,
			    Placeholder)
  %}

%typemap(in) void*[] {
  if (PyList_Check($input)) {
    Py_INCREF($input);
    int size = PyList_Size($input);
    int i = 0;
    int res = 0;
    $1 = (void**)malloc((size+1)*sizeof(HParser*));
    for (i=0; i<size; i++) {
      PyObject *o = PyList_GetItem($input, i);
      res = SWIG_ConvertPtr(o, &($1[i]), SWIGTYPE_p_HParser_, 0 | 0);
      if (!SWIG_IsOK(res)) {
	SWIG_exception_fail(SWIG_ArgError(res), "that wasn't an HParser" );
      }
    }
    $1[size] = NULL;
  } else {
    PyErr_SetString(PyExc_TypeError, "__a functions take lists of parsers as their argument");
    return NULL;
  }
 }
%typemap(in) uint8_t {
  if (PyInt_Check($input)) {
    $1 = PyInt_AsLong($input);
  }
  else if (!PyString_Check($input)) {
    PyErr_SetString(PyExc_ValueError, "Expecting a string");
    return NULL;
  } else {
    $1 = *(uint8_t*)PyString_AsString($input);
  }
 }
%typemap(out) HBytes* {
  $result = PyString_FromStringAndSize((char*)$1->token, $1->len);
 }
%typemap(out) struct HCountedArray_* {
  int i;
  $result = PyList_New($1->used);
  for (i=0; i<$1->used; i++) {
    HParsedToken *t = $1->elements[i];
    PyObject *o = SWIG_NewPointerObj(SWIG_as_voidptr(t), SWIGTYPE_p_HParsedToken_, 0 | 0);
    PyList_SetItem($result, i, o);
  }
 }
%typemap(out) struct HParseResult_* {
  if ($1 == NULL) {
    // TODO: raise parse failure
    Py_INCREF(Py_None);
    $result = Py_None;
  } else {
    $result = hpt_to_python($1->ast);
  }
 }
%typemap(newfree) struct HParseResult_* {
  h_parse_result_free($input);
 }
%inline %{
  static int h_tt_python;
  %}
%init %{
  h_tt_python = h_allocate_token_type("com.upstandinghackers.hammer.python");
  %}




%typemap(in) (HPredicate pred, void* user_data) {
  Py_INCREF($input);
  $2 = $input;
  $1 = call_predicate;
 }

%typemap(in) (const HAction a, void* user_data) {
  Py_INCREF($input);
  $2 = $input;
  $1 = call_action;
 }

%inline %{

  struct HParsedToken_;
  struct HParseResult_;
  static PyObject* hpt_to_python(const struct HParsedToken_ *token);
  
  static struct HParsedToken_* call_action(const struct HParseResult_ *p, void* user_data);
  static int call_predicate(const struct HParseResult_ *p, void* user_data);
 %}
#else
  #warning no uint8_t* typemaps defined
#endif

 // All the include paths are relative to the build, i.e., ../../. If you need to build these manually (i.e., not with scons), keep that in mind.
%{
#include "allocator.h"
#include "hammer.h"
#include "internal.h"
#include "glue.h"
%}
%include "allocator.h"
%include "hammer.h"


%extend HArena_ {
  ~HArena_() {
    h_delete_arena($self);
  }
 };
%extend HParseResult_ {
  ~HParseResult_() {
    h_parse_result_free($self);
  }
};

%newobject h_parse;
%delobject h_parse_result_free;
%newobject h_new_arena;
%delobject h_delete_arena;

#ifdef SWIGPYTHON
%inline {
  static PyObject* hpt_to_python(const HParsedToken *token) {
    // Caller holds a reference to returned object
    PyObject *ret;
    if (token == NULL) {
      Py_RETURN_NONE;
    }
    switch (token->token_type) {
    case TT_NONE:
      return PyObject_CallFunctionObjArgs(_helper_Placeholder, NULL);
      break;
    case TT_BYTES:
      return PyString_FromStringAndSize((char*)token->token_data.bytes.token, token->token_data.bytes.len);
    case TT_SINT:
      // TODO: return PyINT if appropriate
      return PyLong_FromLong(token->token_data.sint);
    case TT_UINT:
      // TODO: return PyINT if appropriate
      return PyLong_FromUnsignedLong(token->token_data.uint);
    case TT_SEQUENCE:
      ret = PyTuple_New(token->token_data.seq->used);
      for (int i = 0; i < token->token_data.seq->used; i++) {
	PyTuple_SET_ITEM(ret, i, hpt_to_python(token->token_data.seq->elements[i]));
      }
      return ret;
    default:
      if (token->token_type == h_tt_python) {
	ret = (PyObject*)token->token_data.user;
	Py_INCREF(ret);
	return ret;
      } else {
	return SWIG_NewPointerObj((void*)token, SWIGTYPE_p_HParsedToken_, 0 | 0);
	// TODO: support registry
      }
      
    }
  }
  static struct HParsedToken_* call_action(const struct HParseResult_ *p, void* user_data) {
    PyObject *callable = user_data;
    PyObject *ret = PyObject_CallFunctionObjArgs(callable,
						 hpt_to_python(p->ast),
						 NULL);
    if (ret == NULL) {
      PyErr_Print();
      assert(ret != NULL);
    }	
    // TODO: add reference to ret to parse-local data
    // For now, just hold onto reference
    HParsedToken *tok = h_make(p->arena, h_tt_python, ret);
    return tok;
   }
  
  static int call_predicate(const struct HParseResult_ *p, void* user_data) {
    PyObject *callable = user_data;
    PyObject *ret = PyObject_CallFunctionObjArgs(callable,
						 hpt_to_python(p->ast),
						 NULL);
    int rret = 0;
    if (ret == NULL) {
      // TODO: throw exception
      PyErr_Print();
      assert(ret != NULL);
    }	
    // TODO: add reference to ret to parse-local data
    rret = PyObject_IsTrue(ret);
    Py_DECREF(ret);
    return rret;
  }
  
 }

%rename("%s") "";

%extend HParser_ {
    HParseResult* parse(const uint8_t* input, size_t length) {
        return h_parse($self, input, length);
    }
    bool compile(HParserBackend backend) {
        return h_compile($self, backend, NULL) == 0;
    }
    PyObject* __dir__() {
        PyObject* ret = PyList_New(2);
        PyList_SET_ITEM(ret, 0, PyString_FromString("parse"));
        PyList_SET_ITEM(ret, 1, PyString_FromString("compile"));
        return ret;
    }
}

%pythoncode %{

def action(p, act):
    return _h_action(p, act)
def attr_bool(p, pred):
    return _h_attr_bool(p, pred)

def ch(ch):
    if isinstance(ch, str) or isinstance(ch, unicode):
        return token(ch)
    else:
        return  _h_ch(ch)

def ch_range(c1, c2):
    dostr = isinstance(c1, str)
    dostr2 = isinstance(c2, str)
    if isinstance(c1, unicode) or isinstance(c2, unicode):
        raise TypeError("ch_range only works on bytes")
    if dostr != dostr2:
        raise TypeError("Both arguments to ch_range must be the same type")
    if dostr:
        return action(_h_ch_range(c1, c2), chr)
    else:
        return _h_ch_range(c1, c2)
def epsilon_p(): return _h_epsilon_p()
def end_p():
    return _h_end_p()
def in_(charset):
    return action(_h_in(charset), chr)
def not_in(charset):
    return action(_h_not_in(charset), chr)
def not_(p): return _h_not(p)
def int_range(p, i1, i2):
    return _h_int_range(p, i1, i2)
def token(string):
    return _h_token(string)
def whitespace(p):
    return _h_whitespace(p)
def xor(p1, p2):
    return _h_xor(p1, p2)
def butnot(p1, p2):
    return _h_butnot(p1, p2)
def and_(p1):
    return _h_and(p1)
def difference(p1, p2):
    return _h_difference(p1, p2)

def sepBy(p, sep): return _h_sepBy(p, sep)
def sepBy1(p, sep): return _h_sepBy1(p, sep)
def many(p): return _h_many(p)
def many1(p): return _h_many1(p)
def repeat_n(p, n): return _h_repeat_n(p, n)
def choice(*args): return _h_choice__a(list(args))
def sequence(*args): return _h_sequence__a(list(args))

def optional(p): return _h_optional(p)
def nothing_p(): return _h_nothing_p()
def ignore(p): return _h_ignore(p)

def left(p1, p2): return _h_left(p1, p2)
def middle(p1, p2, p3): return _h_middle(p1, p2, p3)
def right(p1, p2): return _h_right(p1, p2)

   
class HIndirectParser(_HParser_):
    def __init__(self):
        # Shoves the guts of an _HParser_ into a HIndirectParser.
        tret = _h_indirect()
        self.__dict__.clear()
        self.__dict__.update(tret.__dict__)
        
    def __dir__(self):
        return super(HIndirectParser, self).__dir__() + ['bind']
    def bind(self, parser):
        _h_bind_indirect(self, parser)

def indirect():
    return HIndirectParser()

def bind_indirect(indirect, new_parser):
    indirect.bind(new_parser)

def uint8():  return _h_uint8()
def uint16(): return _h_uint16()
def uint32(): return _h_uint32()
def uint64(): return _h_uint64()
def int8():  return _h_int8()
def int16(): return _h_int16()
def int32(): return _h_int32()
def int64(): return _h_int64()


%}

#endif
