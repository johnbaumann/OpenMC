#ifndef _CLIENT_H
#define _CLIENT_H

namespace openmc
{
    namespace wifi
    {
        namespace client
        {
            void Task_Start(void *params);
            void Init(void);
        } // client
    } // wifi
} // openmc

#endif // _CLIENT_H