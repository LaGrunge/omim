#pragma once

#import <Foundation/Foundation.h>

#include "std/string.hpp"
#include "std/target_os.hpp"

namespace downloader { class IHttpThreadCallback; }

@interface HttpThreadImpl : NSObject

- (instancetype)initWithURL:(string const &)url callback:(downloader::IHttpThreadCallback &)cb
                   begRange:(int64_t)beg endRange:(int64_t)end expectedSize:(int64_t)size postBody:(string const &)pb;

- (void)cancel;

@end
