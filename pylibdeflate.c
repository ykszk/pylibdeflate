#include <Python.h>
#include <libdeflate.h>

PyObject* zlib_compress(PyObject* self, PyObject* args)
{
  void *in;
  int in_nbytes, level = 6;
  if (!PyArg_ParseTuple(args, "y#|i", &in, &in_nbytes, &level)) {
    return NULL;
  }
  struct libdeflate_compressor *compressor = libdeflate_alloc_compressor(level);
  if (compressor == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "compressor allocation failed.");
    return NULL;
  }

  size_t max_compressed_size = libdeflate_zlib_compress_bound(compressor, in_nbytes);
  PyObject* py_bytes = PyBytes_FromStringAndSize(NULL, max_compressed_size);
  if (py_bytes == NULL) {
    libdeflate_free_compressor(compressor);
    return NULL;
  }
  void *compressed_data = PyBytes_AsString(py_bytes);
  size_t compressed_size = libdeflate_zlib_compress(compressor,
    in,
    in_nbytes,
    compressed_data,
    max_compressed_size);
  libdeflate_free_compressor(compressor);
  if (compressed_size == 0) {
    Py_DECREF(py_bytes);
    PyErr_SetString(PyExc_RuntimeError, "Bug in libdeflate_zlib_compress_bound()!");
    return NULL;
  }
  if (_PyBytes_Resize(&py_bytes, compressed_size) == -1) {
    return NULL;
  }
  return py_bytes;
}

PyObject* zlib_decompress(PyObject* self, PyObject* args)
{
  void *in;
  int in_nbytes, decompressed_size;
  if (!PyArg_ParseTuple(args, "y#i", &in, &in_nbytes, &decompressed_size)) {
    return NULL;
  }
  struct libdeflate_decompressor *decompressor = libdeflate_alloc_decompressor();
  if (decompressor == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "decompressor allocation failed.");
    return NULL;
  }

  PyObject* py_bytes = PyBytes_FromStringAndSize(NULL, decompressed_size);
  if (py_bytes == NULL) {
    libdeflate_free_decompressor(decompressor);
    return NULL;
  }
  void *decompressed_data = PyBytes_AsString(py_bytes);
  size_t actual_decompressed_size;
  enum libdeflate_result result = libdeflate_zlib_decompress(decompressor,
    in,
    in_nbytes,
    decompressed_data,
    decompressed_size, &actual_decompressed_size);
  libdeflate_free_decompressor(decompressor);
  if (result != LIBDEFLATE_SUCCESS) {
    Py_DECREF(py_bytes);
    PyErr_SetString(PyExc_RuntimeError, "Corrupted file or not in zlib format");
    return NULL;
  }
  return py_bytes;
}

static PyMethodDef PylibdeflateMethods[] = {
  {"zlib_compress", zlib_compress, METH_VARARGS, "zlib format compression."},
  {"zlib_decompress", zlib_decompress, METH_VARARGS, "zlib format decompression."},
  {NULL, NULL, 0, NULL} /* Sentinel */
};

static struct PyModuleDef pylibdeflate_module = {
  PyModuleDef_HEAD_INIT,
  "pylibdeflate",   /* name of module */
  NULL, /* module documentation, may be NULL */
  -1,
  PylibdeflateMethods
};

PyMODINIT_FUNC
PyInit_pylibdeflate(void)
{
  return PyModule_Create(&pylibdeflate_module);
}