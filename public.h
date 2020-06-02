#ifndef ZSQL_PUBLIC_H
#define ZSQL_PUBLIC_H

#include <string>
#include <iostream>

using namespace std;

#define LOG(str) \
	 cout << __FILE__ <<  ":" << __LINE__ << " "<<  \
	 __TIMESTAMP__ << " : " << str << endl;

#endif //ZSQL_PUBLIC_H
