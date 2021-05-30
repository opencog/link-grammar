/**********************************************************************
*
* SWIG Foreign Function Interface (FFI) definition for the Link Grammar
* shared (dynamic) library.
*
***********************************************************************/
%module clinkgrammar
%{

#include "link-includes.h"
#include "dict-common/dict-defines.h"

%}

%nodefaultdtor lg_errinfo;

#define link_public_api(x) x
#ifndef bool                         /* Prevent syntax errors if no bool. */
#define bool int
#endif /* bool */

%newobject dictionary_get_data_dir;

/* For functions returning (char *), free the returned result
   after it gets converted to a Python object. */
%define %free_returned_value(func)
%newobject func;
%typemap(newfree) char * { free##func($1); }
%ignore free##func; /* They are not a part of the API here. */
%enddef

%free_returned_value(linkage_print_diagram);
%free_returned_value(linkage_print_postscript);
%free_returned_value(linkage_print_links_and_domains);
%free_returned_value(linkage_print_constituent_tree);
%free_returned_value(linkage_print_disjuncts);
%free_returned_value(linkage_print_pp_msgs);

// Reset to default.
%typemap(newfree) char * {
   free($1);
}

%newobject parse_options_create;
%delobject destroy_Parse_Options&;
class Parse_Options {};
%extend Parse_Options
{
   ~Parse_Options()
   {
      parse_options_delete(*$self);
      delete($self);
   }
}
%ignore Parse_Options;

/* Error-handling facility calls. */
%rename(_lg_error_formatmsg) lg_error_formatmsg;
%newobject lg_error_formatmsg;
%rename(_prt_error) prt_error;
/* For security, the first argument should always contain a single "%s"
 * (e.g. "%s\n"), and the second one should always be a C string. */
int prt_error(const char *, const char *);

/*
 * A wrapper to this function is complex and is not implemented here.  However,
 * such a wrapper may not be needed anyway since this function is provided
 * mainly for the low-level implementation of the error callback, so bound
 * languages can free the memory of the callback data.
 */
%ignore lg_error_set_handler_data;

%immutable;                          /* Future-proof for const definitions. */
%include link-includes.h
%include dict-common/dict-defines.h
%mutable;

#ifdef SWIGPYTHON
%extend lg_errinfo
{
    %pythoncode
    %{
        def formatmsg(self):
            return _lg_error_formatmsg(self)
        __swig_destroy__ = _clinkgrammar.delete_lg_errinfo
        __del__ = lambda self: None
    %}
}

%{
static lg_error_handler default_error_handler;

static lg_errinfo *dup_lg_errinfo(lg_errinfo *lge)
{
   lg_errinfo *mlge = (lg_errinfo *)malloc(sizeof(lg_errinfo));
   mlge->severity_label = strdup(lge->severity_label);
   mlge->text = strdup(lge->text);
   mlge->severity = lge->severity;

   return mlge;
}

/**
 * This function is installed as the C error callback when an error callback
 * is set by the Python code to a Python function (but not when set to None
 * or to the library default error handler).
 * When invoked by the LG library, it calls the Python function along with
 * its data. Both appear in func_and_data, which is a Python tuple of 2
 * elements - a function and an arbitrary data object.
*/
static void PythonCallBack(lg_errinfo *lge, void *func_and_data)
{
   lg_errinfo *mlgep = dup_lg_errinfo(lge);
   PyObject *pylge = SWIG_NewPointerObj(SWIG_as_voidptr(mlgep),
                                       SWIGTYPE_p_lg_errinfo, SWIG_POINTER_OWN);
   PyObject *func = PyTuple_GetItem((PyObject *)func_and_data, 0);
   PyObject *data = PyTuple_GetItem((PyObject *)func_and_data, 1);

   PyObject *args = Py_BuildValue("OO", pylge, data);
   PyObject *rc = PyEval_CallObject(func, args); /* Py LG error cb. */

   Py_DECREF(pylge);
   Py_DECREF(args);
   if (NULL == rc)
       PyErr_Print();
   Py_XDECREF(rc);
}
%}

%typemap(in) lg_errinfo *eh_lge
{
   void *argp1 = 0;

   if (Py_None == $input)
      SWIG_exception_fail(SWIG_TypeError, "in method '_py_error_default_handler', argument 1 (of type lg_errinfo *) must not be None.");

   int res1 = SWIG_ConvertPtr($input, &argp1, SWIGTYPE_p_lg_errinfo, 0);
   if (!SWIG_IsOK(res1)) {
      SWIG_exception_fail(SWIG_ArgError(res1), "in method '_py_error_default_handler', argument 1 of type 'lg_errinfo *'");
   }
   arg1 = (lg_errinfo *)(argp1);
}

/* The second argument of the default callback can be NULL or
   a severity_level integer. Validate that and convert it to C int. */
%typemap(in) int *pedh_data (int arg)
{
   bool error = false;
   const char errmsg[] = "The default error handler data argument (arg 2) "
                         "must be an integer (0 to lg_None) or None.";

   if (Py_None == $input)
   {
      $1 = NULL;
   }
   else
   {
      if (!PyInt_Check($input))
      {
         SWIG_exception_fail(SWIG_TypeError, errmsg);
         error = true;
      }
      else
      {
          arg = (int)PyInt_AsLong($input);
      }

      if ((arg < 0) || (arg > lg_None))
      {
         SWIG_exception_fail(SWIG_ValueError, errmsg);
         error = true;
      }

      if (error) return NULL;
      $1 = &arg;
   }
}

%inline %{
void _py_error_default_handler(lg_errinfo *eh_lge, int *pedh_data)
{
    default_error_handler(eh_lge, (void *)pedh_data);
}

/**
 * Set a Python function/data as the LG error handler callback.
 * Note that because the LG library cannot directly call a Python function,
 * the actual callback function is a C proxy function PythonCallBack() and
 * the Python function/data is set as the C callback data.
 */
PyObject *_py_error_set_handler(PyObject *func_and_data)
{
   const void *old_func_and_data = lg_error_set_handler_data(NULL);
   PyObject *func = PyTuple_GetItem((PyObject *)func_and_data, 0);
   lg_error_handler old_handler;

   if (Py_None == func)
   {
      old_handler = lg_error_set_handler(NULL, NULL);
   }
   else
   {
      if (!PyCallable_Check(func)) {
          PyErr_SetString(PyExc_TypeError, "Argument 1 must be callable");
          return NULL;
      }
      old_handler = lg_error_set_handler(PythonCallBack, func_and_data);
      Py_INCREF(func_and_data);
   }

   if (NULL == (PyObject *)old_handler)
      Py_RETURN_NONE;

   if (PythonCallBack == old_handler)
   {
      func = PyTuple_GetItem((PyObject *)old_func_and_data, 0);
      Py_INCREF(func);
      Py_XDECREF(old_func_and_data);
      return func;
   }

   /* This must be the first call. Grab the C default error handler. */
   default_error_handler = old_handler;

   /* Signify this is the default error handler by a string object. */
   return Py_BuildValue("s", "");
}

PyObject *_py_error_printall(PyObject *func_and_data)
{
   Py_INCREF(func_and_data);
   int n = lg_error_printall(PythonCallBack, func_and_data);
   Py_DECREF(func_and_data);

   PyObject *py_n = PyInt_FromLong(n);
   return py_n;
}

void delete_lg_errinfo(lg_errinfo *lge) {
  if (NULL == lge) return; /* Was None - nothing to free. */
  free((void *)lge->severity_label);
  free((void *)lge->text);
  free((void *)lge);
}

/**
 * incref/decref a Python object.
 * Currently used on the Dictionary object when a Sentence object is created/deleted,
 * because the Sentence object includes a reference to the Dictionary structure.
 */
void _py_incref(PyObject *x)
{
  Py_INCREF(x);
}

void _py_decref(PyObject *x)
{
  Py_DECREF(x);
}
%}
#endif /* SWIGPYTHON */
