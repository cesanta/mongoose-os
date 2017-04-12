package main

import (
	"log"
	"os"

	"github.com/alexflint/gallium"
)

/*
#cgo CFLAGS: -x objective-c
#cgo CFLAGS: -framework Cocoa
#cgo LDFLAGS: -framework Cocoa

#include <Cocoa/Cocoa.h>
#include <dispatch/dispatch.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>

@property (assign) NSWindow *window;

@end

@implementation AppDelegate

@synthesize window;

- (id)init{
    self = [super init];
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication {
    return YES;
}

@end

void setTerminateAfterLastWindowClosed() {
  dispatch_async(dispatch_get_main_queue(), ^{
	[[NSApplication sharedApplication] setDelegate: [[AppDelegate alloc]init]];
  });
}
*/
import "C"

func showUI(url string) {
	err := gallium.Loop(os.Args, func(app *gallium.App) {
		opts := gallium.FramedWindow
		opts.Title = "Mongoose OS"
		opts.Shape = gallium.Rect{
			Width:  1280,
			Height: 720,
			Left:   320,
			Bottom: 180,
		}
		_, err := app.OpenWindow(url, opts)
		if err != nil {
			log.Fatal(err)
		}
		C.setTerminateAfterLastWindowClosed()
		app.SetMenu([]gallium.Menu{
			{
				Title: "Mongoose OS",
				Entries: []gallium.MenuEntry{
					gallium.MenuItem{
						Title:    "Quit",
						Shortcut: gallium.MustParseKeys("cmd q"),
						OnClick:  func() { os.Exit(0) },
					},
				},
			},
		})
	})
	if err != nil {
		log.Fatal(err)
	}
}
