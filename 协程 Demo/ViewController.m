//
//  ViewController.m
//  协程 Demo
//
//  Created by 张鸿 on 2017/11/21.
//  Copyright © 2017年 酸菜Amour. All rights reserved.
//

#import "ViewController.h"
#import "Coroutine.h"

@interface ViewController ()

@end

@implementation ViewController

void test1(void)
{
    int i = 0;
    char array[5000];
    while (1) {
        NSLog(@"test1   %d  %p", i, array);
        i++;
        sleep(1);
        coroutine_switch();
    }
}


void test3(void)
{
    int i = 0;
    coroutine_create(test1);
    while (1) {
        NSLog(@"test3 张鸿  %d", i * 10);
        i++;
        sleep(1);
//        coroutine_switch();
        if (i != 10) {
            coroutine_switch();
        } else {
            coroutine_release();
        }
    }
}

void test2(void)
{
    int i = 1;
    char array[50];
    while (1) {
        NSLog(@"test2   %d  %p", i, array);
        i++;
        sleep(1);
        if (i != 10) {
            coroutine_switch();
        } else {
            coroutine_release();
        }
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Do any additional setup after loading the view, typically from a nib.
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        coroutine_start(test2);
        NSLog(@"协程任务结束");
    });
    
    coroutine_start(test3);
    
    NSLog(@"哈哈哈");

}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    NSLog(@"%@", NSStringFromSelector(_cmd));
}


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
