/*
  file: trocar_autor.c
*/
#include <stdio.h>
#include <python2.7/Python.h>

PyObject *pName, *pModule, *pDict, *pFunc, *pValue, *pArgs, *pClass, *pInstance;
int EndPython(){
    Py_XDECREF(pInstance); //using XDECREF instead of DECREF to avoid problems is pInstance is NULL
    Py_XDECREF(pValue);
    Py_XDECREF(pModule);
    Py_XDECREF(pName);
    Py_Finalize();
    return 1;
}

int main(){

    PyObject *main_module, *dict;
    PyObject *wifi;//, *resultado;


    char mode[200];
    char apname[200];

//    char *c_resultado;
    
    FILE *python;

    printf("Arquivo de entrada\n");
    scanf("%200[^\n]%*c",mode);

    printf("Arquivo de saida\n");
    scanf("%200[^\n]%*c",apname);

    Py_Initialize();
    main_module = PyImport_AddModule("__main__");
    dict = PyModule_GetDict(main_module);

       python = fopen("wifi.py", "r");
    PyRun_SimpleFile(python, "wifi.py");



    pName = PyString_FromString("accessPoint");

    printf("Return of call 5: %s\n", PyString_AsString(pName));


    PyObject* pModule = PyImport_Import(python,pName);
    printf("xxx");

    pDict = PyModule_GetDict(pModule);




    printf("passou");


    
    wifi = PyDict_GetItemString(dict, "accessPoint");
//    wifi = PyDict_GetItemString(dict, "mobility");
//    printf("%s\n",mode);
    //resultado = PyObject_CallFunction(wifi,mode);

//    if (PyCallable_Check(wifi)) {//no such class or class not callable
        pInstance = PyObject_CallObject(wifi, NULL);//geting an instance. new reference. Arguents for the constructor are NULL
  //  }else{
  //      return EndPython();
   // }

    pValue = PyObject_CallMethod(pInstance, "returnApMode", NULL);
//    pValue = PyObject_CallMethod(pInstance, "range", "(s)", mode);
    printf("Return of call 5: %s\n", PyString_AsString(pValue));
//    printf("Return of call 5: %ld\n", PyInt_AsLong(pValue));

//    resultado = PyObject_CallFunction(wifi,"ssss",mode);
    printf("passou");
    
   // c_resultado = PyString_AsString(resultado);

    Py_Finalize();

    printf("--Texto Formatado--\n");
   // printf("%s\n",c_resultado);

    return 0;
}
