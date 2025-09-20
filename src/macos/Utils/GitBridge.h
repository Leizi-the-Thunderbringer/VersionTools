#import <Foundation/Foundation.h>

// C++ to Swift bridge for Git operations
@interface GitBridge : NSObject

- (instancetype)init;
- (BOOL)openRepository:(NSString *)path;
- (NSArray *)getFileChanges;
- (NSArray *)getCommitHistory:(int)maxCount;
- (NSArray *)getBranches;
- (NSDictionary *)getRepositoryStatus;
- (NSArray *)getCommitChanges:(NSString *)commitHash;
- (NSDictionary *)getFileDiff:(NSString *)filePath commitHash:(NSString *)commitHash;
- (NSArray *)getBranchCommits:(NSString *)branchName maxCount:(int)maxCount;

// File operations
- (BOOL)stageFile:(NSString *)filePath;
- (BOOL)unstageFile:(NSString *)filePath;
- (BOOL)stageAllFiles;
- (BOOL)unstageAllFiles;

// Commit operations
- (BOOL)commit:(NSString *)message;

// Branch operations
- (BOOL)checkoutBranch:(NSString *)branchName;
- (BOOL)createBranch:(NSString *)branchName;
- (BOOL)deleteBranch:(NSString *)branchName;

@end