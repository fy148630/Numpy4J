#include <jni.h>
#include <stdio.h>
#include "JNIBridge.h"
#include <Python.h>

const char* jNP_ARRAY_NAME = "np/NPArray";
const char* jNP_ARRAY_BUFFER_FIELD = "buffer";
const char* jBYTE_BUFFER_NAME = "java/nio/ByteBuffer";
const char* jBYTE_BUFFER_TYPE = "Ljava/nio/ByteBuffer;";
jfieldID BUFFER_FID;

PyObject *npModule, *dtypeFunc, *fromBufferFunc;
const char* NP_MODULE = "numpy";
const char* NP_DTYPE_CONSTRUCTOR = "dtype";
const char* NP_FROM_BUFFER = "frombuffer";

PyObject *NP_INT32;
PyObject *npMaxFunc;

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  //Safely cache relevant java class information
  JNIEnv *env;
  (*vm)->AttachCurrentThread(vm, (void **) &env, NULL);

  jclass classNPArray = (*env)->FindClass(env, jNP_ARRAY_NAME);
  jclass classByteBuffer = (*env)->FindClass(env, jBYTE_BUFFER_NAME);
  BUFFER_FID = (*env)->GetFieldID(env, classNPArray, jNP_ARRAY_BUFFER_FIELD, jBYTE_BUFFER_TYPE);
  (*vm)->DetachCurrentThread(vm);

  //Setup python environment, acquire required functions
  Py_SetProgramName("numpy4J (Bridge)");  /* optional but recommended */
  Py_Initialize();
  npModule = PyImport_Import(PyString_FromString(NP_MODULE));
  dtypeFunc = PyObject_GetAttrString(npModule, NP_DTYPE_CONSTRUCTOR);
  fromBufferFunc = PyObject_GetAttrString(npModule, NP_FROM_BUFFER);

  NP_INT32 = PyObject_GetAttrString(npModule, "int32");
  npMaxFunc = PyObject_GetAttrString(npModule, "max");

  return JNI_VERSION_1_6; //TODO: 1.2 is "any JNI version"...might be able to use that instead



}

void JNI_OnUnload(JavaVM *vm, void *reserved) {Py_Finalize();}

//Investigate caching these dtypes as they will likely be re-used
JNIEXPORT jint JNICALL Java_np_JNIBridge_max
(JNIEnv *env, jobject this, jobject jnparray) {
  //Acquire buffer information 
  jobject bufferRef = (*env)->GetObjectField(env, jnparray, BUFFER_FID);
  jbyte* jbuffer = (*env)->GetDirectBufferAddress(env, bufferRef);
  jlong jsize = (*env)->GetDirectBufferCapacity(env, bufferRef);
  
  //Create a python buffer from the java buffer
  Py_ssize_t size = PyInt_AsSsize_t(PyInt_FromLong(jsize)); 
  PyObject *pybuffer = PyBuffer_FromReadWriteMemory(jbuffer, size);

  //Create np dtype object, based on the passed array's dtype information
  PyObject *dtypeArgs = PyTuple_Pack(1, NP_INT32); //TODO: Acquire and inspect the passed array's dtype information; 
  PyObject *dtype = PyObject_CallObject(dtypeFunc, dtypeArgs);

  //Create numpy array
  PyObject *arrayArgs = PyTuple_Pack(2, pybuffer, dtype);
  PyObject *nparray = PyObject_CallObject(fromBufferFunc, arrayArgs);

  //Invoke max
  PyObject *maxArgs = PyTuple_Pack(1, nparray);
  PyObject *max = PyObject_CallObject(npMaxFunc, maxArgs);
  
  //Cleanup for return
  long rv = PyInt_AsLong(max);
  Py_DECREF(dtype);
  Py_DECREF(arrayArgs);
  Py_DECREF(dtypeArgs);
  Py_DECREF(nparray);
  Py_DECREF(maxArgs);
  Py_DECREF(max);

  return (jint) rv;
}
