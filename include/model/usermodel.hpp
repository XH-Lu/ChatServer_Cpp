#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"


class UserModel
{
private:

public:
    bool insert(User& user);
    User query(string name);
    User query(int id);
    bool update(User user);
    void resetState();

};




#endif