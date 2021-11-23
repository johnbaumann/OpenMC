#ifndef _ACCESS_POINT_H
#define _ACCESS_POINT_H

namespace openmc
{
    namespace wifi
    {
        namespace access_point
        {
            void Init(void);
            void Task_Start(void *params);
        } // access_point

    } // wifi
} // openmc

#endif // _ACCESS_POINT_H