//
//  Coroutine.h
//  协程 Demo
//
//  Created by 张鸿 on 2017/11/28.
//  Copyright © 2017年 酸菜Amour. All rights reserved.
//

#ifndef Coroutine_h
#define Coroutine_h

#include <stdio.h>

typedef void (*coroutineTask)(void);

void coroutine_switch(void);
void coroutine_release(void);
void coroutine_start(coroutineTask entryTask);
void coroutine_create(coroutineTask task);

#endif /* Coroutine_h */
