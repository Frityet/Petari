#pragma once

class FileLoader;

namespace smgpc::compat {

// The process must stop submitting file requests before retirement and keep
// every original target heap alive through the final archive unmount.
// A matching original singleton is cleared after its worker has joined.
void destroy_file_loader(FileLoader*);

}
