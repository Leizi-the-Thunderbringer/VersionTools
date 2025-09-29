#import "GitBridge.h"
#include "core/GitManager.h"
#include "core/GitUtils.h"
#include "core/SystemCommand.h"
#include <memory>

using namespace VersionTools;

@interface GitBridge() {
    std::unique_ptr<GitManager> gitManager;
}
@end

@implementation GitBridge

- (instancetype)init {
    self = [super init];
    if (self) {
        gitManager = std::make_unique<GitManager>();
    }
    return self;
}

- (BOOL)openRepository:(NSString *)path {
    std::string cppPath = [path UTF8String];
    auto result = gitManager->openRepository(cppPath);
    return result.isSuccess();
}

- (NSArray *)getFileChanges {
    auto status = gitManager->getStatus();
    NSMutableArray *changes = [NSMutableArray array];
    
    for (const auto& change : status.changes) {
        NSString *filePath = [NSString stringWithUTF8String:change.filePath.c_str()];
        NSString *fileName = [NSString stringWithUTF8String:GitUtils::getFileName(change.filePath).c_str()];
        NSString *dirPath = [NSString stringWithUTF8String:GitUtils::getDirectory(change.filePath).c_str()];
        
        NSDictionary *changeDict = @{
            @"filePath": filePath,
            @"fileName": fileName,
            @"directoryPath": dirPath,
            @"status": @((int)change.status),
            @"isStaged": @(change.isStaged),
            @"linesAdded": @(change.linesAdded),
            @"linesDeleted": @(change.linesDeleted)
        };
        
        [changes addObject:changeDict];
    }
    
    return changes;
}

- (NSArray *)getCommitHistory:(int)maxCount {
    auto commits = gitManager->getCommitHistory(maxCount);
    NSMutableArray *commitArray = [NSMutableArray array];
    
    for (const auto& commit : commits) {
        NSString *hash = [NSString stringWithUTF8String:commit.hash.c_str()];
        NSString *shortHash = [NSString stringWithUTF8String:commit.shortHash.c_str()];
        NSString *author = [NSString stringWithUTF8String:commit.author.c_str()];
        NSString *email = [NSString stringWithUTF8String:commit.email.c_str()];
        NSString *message = [NSString stringWithUTF8String:commit.message.c_str()];
        NSString *shortMessage = [NSString stringWithUTF8String:commit.shortMessage.c_str()];
        
        auto timeT = std::chrono::system_clock::to_time_t(commit.timestamp);
        NSDate *date = [NSDate dateWithTimeIntervalSince1970:timeT];
        
        NSMutableArray *parents = [NSMutableArray array];
        for (const auto& parent : commit.parentHashes) {
            [parents addObject:[NSString stringWithUTF8String:parent.c_str()]];
        }
        
        NSDictionary *commitDict = @{
            @"hash": hash,
            @"shortHash": shortHash,
            @"author": author,
            @"email": email,
            @"message": message,
            @"shortMessage": shortMessage,
            @"timestamp": date,
            @"parentHashes": parents,
            @"isMerge": @(commit.isMerge())
        };
        
        [commitArray addObject:commitDict];
    }
    
    return commitArray;
}

- (NSArray *)getBranches {
    auto branches = gitManager->getBranches(true);
    NSMutableArray *branchArray = [NSMutableArray array];
    
    for (const auto& branch : branches) {
        NSString *name = [NSString stringWithUTF8String:branch.name.c_str()];
        NSString *fullName = [NSString stringWithUTF8String:branch.fullName.c_str()];
        NSString *displayName = [NSString stringWithUTF8String:GitUtils::getShortBranchName(branch.fullName).c_str()];
        NSString *upstream = branch.upstreamBranch.empty() ? nil : [NSString stringWithUTF8String:branch.upstreamBranch.c_str()];
        
        NSDictionary *lastCommitDict = nil;
        if (branch.lastCommit.has_value()) {
            const auto& commit = branch.lastCommit.value();
            auto timeT = std::chrono::system_clock::to_time_t(commit.timestamp);
            NSDate *date = [NSDate dateWithTimeIntervalSince1970:timeT];
            
            lastCommitDict = @{
                @"hash": [NSString stringWithUTF8String:commit.hash.c_str()],
                @"shortHash": [NSString stringWithUTF8String:commit.shortHash.c_str()],
                @"author": [NSString stringWithUTF8String:commit.author.c_str()],
                @"message": [NSString stringWithUTF8String:commit.shortMessage.c_str()],
                @"timestamp": date
            };
        }
        
        NSDictionary *branchDict = @{
            @"name": name,
            @"fullName": fullName,
            @"displayName": displayName,
            @"isRemote": @(branch.isRemote),
            @"isCurrent": @(branch.isCurrent),
            @"upstreamBranch": upstream ?: [NSNull null],
            @"aheadCount": @(branch.aheadCount),
            @"behindCount": @(branch.behindCount),
            @"lastCommit": lastCommitDict ?: [NSNull null]
        };
        
        [branchArray addObject:branchDict];
    }
    
    return branchArray;
}

- (NSDictionary *)getRepositoryStatus {
    auto status = gitManager->getStatus();
    
    NSString *currentBranch = [NSString stringWithUTF8String:status.currentBranch.c_str()];
    NSString *upstreamBranch = status.upstreamBranch.empty() ? nil : [NSString stringWithUTF8String:status.upstreamBranch.c_str()];
    
    int stagedCount = 0, unstagedCount = 0;
    for (const auto& change : status.changes) {
        if (change.isStaged) {
            stagedCount++;
        } else {
            unstagedCount++;
        }
    }
    
    // Get stash count
    auto stashes = gitManager->getStashes();
    
    return @{
        @"currentBranch": currentBranch,
        @"upstreamBranch": upstreamBranch ?: [NSNull null],
        @"aheadCount": @(status.aheadCount),
        @"behindCount": @(status.behindCount),
        @"stagedChangesCount": @(stagedCount),
        @"unstagedChangesCount": @(unstagedCount),
        @"stashCount": @(stashes.size()),
        @"hasUncommittedChanges": @(status.hasUncommittedChanges)
    };
}

- (NSArray *)getCommitChanges:(NSString *)commitHash {
    std::string hash = [commitHash UTF8String];
    auto diffs = gitManager->getCommitDiffAll(hash);
    NSMutableArray *changes = [NSMutableArray array];
    
    for (const auto& diff : diffs) {
        NSString *filePath = [NSString stringWithUTF8String:diff.filePath.c_str()];
        NSString *fileName = [NSString stringWithUTF8String:GitUtils::getFileName(diff.filePath).c_str()];
        NSString *dirPath = [NSString stringWithUTF8String:GitUtils::getDirectory(diff.filePath).c_str()];
        
        int linesAdded = 0, linesDeleted = 0;
        for (const auto& hunk : diff.hunks) {
            for (const auto& line : hunk.lines) {
                if (line.type == GitDiffLine::Type::Addition) {
                    linesAdded++;
                } else if (line.type == GitDiffLine::Type::Deletion) {
                    linesDeleted++;
                }
            }
        }
        
        FileStatus status = FileStatus::Modified;
        if (diff.isNewFile) status = FileStatus::Added;
        else if (diff.isDeletedFile) status = FileStatus::Deleted;
        
        NSDictionary *changeDict = @{
            @"filePath": filePath,
            @"fileName": fileName,
            @"directoryPath": dirPath,
            @"status": @((int)status),
            @"isStaged": @(true), // Committed changes are considered "staged"
            @"linesAdded": @(linesAdded),
            @"linesDeleted": @(linesDeleted)
        };
        
        [changes addObject:changeDict];
    }
    
    return changes;
}

- (NSDictionary *)getFileDiff:(NSString *)filePath commitHash:(NSString *)commitHash {
    std::string path = [filePath UTF8String];
    std::string hash = [commitHash UTF8String];
    
    auto diff = gitManager->getCommitDiff(hash);
    if (diff.filePath != path) {
        // If not found in single file diff, try getting from all diffs
        auto allDiffs = gitManager->getCommitDiffAll(hash);
        bool found = false;
        for (const auto& d : allDiffs) {
            if (d.filePath == path) {
                diff = d;
                found = true;
                break;
            }
        }
        if (!found) {
            return nil;
        }
    }
    
    NSMutableArray *hunks = [NSMutableArray array];
    for (const auto& hunk : diff.hunks) {
        NSMutableArray *lines = [NSMutableArray array];
        
        for (const auto& line : hunk.lines) {
            int lineType = 0; // context
            if (line.type == GitDiffLine::Type::Addition) lineType = 1;
            else if (line.type == GitDiffLine::Type::Deletion) lineType = 2;
            else if (line.type == GitDiffLine::Type::Header) lineType = 3;
            
            NSDictionary *lineDict = @{
                @"type": @(lineType),
                @"content": [NSString stringWithUTF8String:line.content.c_str()],
                @"oldLineNumber": @(line.oldLineNumber),
                @"newLineNumber": @(line.newLineNumber)
            };
            
            [lines addObject:lineDict];
        }
        
        NSDictionary *hunkDict = @{
            @"header": [NSString stringWithUTF8String:hunk.header.c_str()],
            @"oldStart": @(hunk.oldStart),
            @"oldCount": @(hunk.oldCount),
            @"newStart": @(hunk.newStart),
            @"newCount": @(hunk.newCount),
            @"lines": lines
        };
        
        [hunks addObject:hunkDict];
    }
    
    // Generate raw content for copying
    std::string rawContent;
    for (const auto& hunk : diff.hunks) {
        rawContent += hunk.header + "\n";
        for (const auto& line : hunk.lines) {
            if (line.type == GitDiffLine::Type::Addition) {
                rawContent += "+" + line.content + "\n";
            } else if (line.type == GitDiffLine::Type::Deletion) {
                rawContent += "-" + line.content + "\n";
            } else {
                rawContent += " " + line.content + "\n";
            }
        }
    }
    
    return @{
        @"filePath": [NSString stringWithUTF8String:diff.filePath.c_str()],
        @"isBinary": @(diff.isBinary),
        @"isNewFile": @(diff.isNewFile),
        @"isDeletedFile": @(diff.isDeletedFile),
        @"hunks": hunks,
        @"rawContent": [NSString stringWithUTF8String:rawContent.c_str()]
    };
}

- (NSArray *)getBranchCommits:(NSString *)branchName maxCount:(int)maxCount {
    std::string branch = [branchName UTF8String];
    auto commits = gitManager->getCommitHistory(maxCount, GitLogOptions::None, branch);
    
    return [self convertCommitsToArray:commits];
}

// File operations
- (BOOL)stageFile:(NSString *)filePath {
    std::string path = [filePath UTF8String];
    auto result = gitManager->addFiles({path});
    return result.isSuccess();
}

- (BOOL)unstageFile:(NSString *)filePath {
    std::string path = [filePath UTF8String];
    auto result = gitManager->resetFiles({path});
    return result.isSuccess();
}

- (BOOL)stageAllFiles {
    auto result = gitManager->addAllFiles();
    return result.isSuccess();
}

- (BOOL)unstageAllFiles {
    auto result = gitManager->resetFiles({});
    return result.isSuccess();
}

// Commit operations
- (BOOL)commit:(NSString *)message {
    std::string msg = [message UTF8String];
    auto result = gitManager->commit(msg);
    return result.isSuccess();
}

// Branch operations
- (BOOL)checkoutBranch:(NSString *)branchName {
    std::string name = [branchName UTF8String];
    auto result = gitManager->checkoutBranch(name);
    return result.isSuccess();
}

- (BOOL)createBranch:(NSString *)branchName {
    std::string name = [branchName UTF8String];
    auto result = gitManager->createBranch(name);
    return result.isSuccess();
}

- (BOOL)createBranch:(NSString *)branchName fromCommit:(NSString *)startPoint {
    std::string name = [branchName UTF8String];
    std::string start = startPoint ? [startPoint UTF8String] : "";
    auto result = gitManager->createBranch(name, start);
    return result.isSuccess();
}

- (BOOL)deleteBranch:(NSString *)branchName {
    std::string name = [branchName UTF8String];
    auto result = gitManager->deleteBranch(name, false);
    return result.isSuccess();
}

- (BOOL)deleteBranch:(NSString *)branchName force:(BOOL)force {
    std::string name = [branchName UTF8String];
    auto result = gitManager->deleteBranch(name, force);
    return result.isSuccess();
}

// Stash operations
- (NSArray *)getStashes {
    auto stashes = gitManager->getStashes();
    NSMutableArray *stashArray = [NSMutableArray array];

    for (const auto& stash : stashes) {
        auto timeT = std::chrono::system_clock::to_time_t(stash.timestamp);
        NSDate *date = [NSDate dateWithTimeIntervalSince1970:timeT];

        NSDictionary *stashDict = @{
            @"name": [NSString stringWithUTF8String:stash.name.c_str()],
            @"message": [NSString stringWithUTF8String:stash.message.c_str()],
            @"branch": stash.branch.empty() ? [NSNull null] : [NSString stringWithUTF8String:stash.branch.c_str()],
            @"timestamp": date,
            @"index": @(stash.index)
        };

        [stashArray addObject:stashDict];
    }

    return stashArray;
}

- (BOOL)createStash:(NSString *)message {
    std::string msg = message ? [message UTF8String] : "";
    auto result = gitManager->stash(msg, false);
    return result.isSuccess();
}

- (BOOL)applyStash:(int)index {
    auto result = gitManager->stashApply(index);
    return result.isSuccess();
}

- (BOOL)popStash:(int)index {
    auto result = gitManager->stashPop(index);
    return result.isSuccess();
}

- (BOOL)dropStash:(int)index {
    auto result = gitManager->stashDrop(index);
    return result.isSuccess();
}

// Raw command execution
- (NSString *)executeRawCommand:(NSString *)command {
    if (!gitManager) {
        return nil;
    }

    std::string cmd = [command UTF8String];
    SystemCommand systemCmd;
    auto result = systemCmd.execute(cmd, {}, gitManager->getRepositoryPath());

    if (result.exitCode == 0) {
        return [NSString stringWithUTF8String:result.output.c_str()];
    }

    // Return error as output if command failed
    std::string errorOutput = result.error.empty() ? result.output : result.error;
    return [NSString stringWithUTF8String:errorOutput.c_str()];
}

// Helper method to convert commits to NSArray
- (NSArray *)convertCommitsToArray:(const std::vector<GitCommit>&)commits {
    NSMutableArray *commitArray = [NSMutableArray array];
    
    for (const auto& commit : commits) {
        NSString *hash = [NSString stringWithUTF8String:commit.hash.c_str()];
        NSString *shortHash = [NSString stringWithUTF8String:commit.shortHash.c_str()];
        NSString *author = [NSString stringWithUTF8String:commit.author.c_str()];
        NSString *email = [NSString stringWithUTF8String:commit.email.c_str()];
        NSString *message = [NSString stringWithUTF8String:commit.message.c_str()];
        NSString *shortMessage = [NSString stringWithUTF8String:commit.shortMessage.c_str()];
        
        auto timeT = std::chrono::system_clock::to_time_t(commit.timestamp);
        NSDate *date = [NSDate dateWithTimeIntervalSince1970:timeT];
        
        NSMutableArray *parents = [NSMutableArray array];
        for (const auto& parent : commit.parentHashes) {
            [parents addObject:[NSString stringWithUTF8String:parent.c_str()]];
        }
        
        NSDictionary *commitDict = @{
            @"hash": hash,
            @"shortHash": shortHash,
            @"author": author,
            @"email": email,
            @"message": message,
            @"shortMessage": shortMessage,
            @"timestamp": date,
            @"parentHashes": parents,
            @"isMerge": @(commit.isMerge())
        };
        
        [commitArray addObject:commitDict];
    }
    
    return commitArray;
}

@end