//
//  ViewController.m
//  ExecDill
//
//  Created by mac on 2021/3/24.
//

#import "ViewController.h"
#include "include/dart_api.h"
#include "include/dart_embedder_api.h"
#include <vector>
#include <cstdlib>
#include "bin/dartutils.h"

@interface ViewController ()

@end

@implementation ViewController

Dart_Handle NewString(const char* str) {
  return Dart_NewStringFromCString(str);
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
//    [self downloadBundle];
    
// !!!! download libdartvm_with_utils.a and add to project! https://mmm-1254409884.cos.ap-guangzhou.myqcloud.com/libdartvm_with_utils.a
    NSString *path = [[NSBundle mainBundle] pathForResource:@"a" ofType:@"dill"];
    [self loadDill:path];
}

-(void) initDart{
    char* error = nullptr;
    if (!dart::embedder::InitOnce(&error)) {
        NSLog(@"AA:%s",error);
        free(error);
        abort();
      }
    std::vector<const char*> flags{};
      error = Dart_SetVMFlags(flags.size(), flags.data());
      if (error != nullptr) {
          NSLog(@"2");
        abort();
      }
      Dart_InitializeParams init_params;
      memset(&init_params, 0, sizeof(init_params));
      init_params.version = DART_INITIALIZE_PARAMS_CURRENT_VERSION;
      error = Dart_Initialize(&init_params);
      if (error != nullptr) {
        dart::embedder::Cleanup();
          NSLog(@"3");
        free(error);
        abort();
      }
    NSString *path = [[NSBundle mainBundle] pathForResource:@"vm_platform_strong" ofType:@"dill"];
    NSData *vmData = [NSData dataWithContentsOfFile:path];
    Dart_Isolate isolate = Dart_CreateIsolateGroupFromKernel(path.UTF8String,"main",(UInt8 *)vmData.bytes,vmData.length,nullptr,nullptr,nullptr,&error);
     if (isolate == nullptr) {
         NSLog(@"5");
       abort();
     }
     Dart_EnterScope();
    dart::bin::DartUtils::PrepareForScriptLoading(false, false);
    NSLog(@"6");

}
-(void) downloadBundle{
    NSLog(@"正在下载文件");
    NSString *url = @"https://mmm-1254409884.cos.ap-guangzhou.myqcloud.com/a.dill";
   NSURLSessionDataTask *task = [[NSURLSession sharedSession] dataTaskWithURL:[NSURL URLWithString:url] completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        NSLog(@"下载成功:%f",data.length/1000000.0);
        if (error) {
            NSLog(@"error:%@",error);
        }

        NSString *destPath = [NSString stringWithFormat:@"%@/Documents/a.dill",NSHomeDirectory()];
       if ([[NSFileManager defaultManager] fileExistsAtPath:destPath]) {
           [[NSFileManager defaultManager] removeItemAtPath:destPath error:nil];
       }
        [data writeToFile:destPath options:0 error:nil];
        NSLog(@"%@",destPath);
       sleep(1);
       [[NSOperationQueue mainQueue] addOperationWithBlock:^{
           [self loadDill:destPath];
       }];
    }];
    [task resume];
}
-(void) loadDill:(NSString *)dillPath{
    [self initDart];
    NSData *data = [NSData dataWithContentsOfFile:dillPath];
    Dart_Handle main_libraray = Dart_LoadLibraryFromKernel((UInt8 *)data.bytes, data.length);
    Dart_SetRootLibrary(main_libraray);
    Dart_Invoke(main_libraray, NewString("main"), 0, nullptr);
    Dart_RunLoop();
    Dart_ShutdownIsolate();
    Dart_Cleanup();
}

@end
