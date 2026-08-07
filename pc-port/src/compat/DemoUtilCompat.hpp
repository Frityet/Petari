#pragma once

namespace smgpc::compat {

    // Opening-demo teardown enters through PlayerUtil, but the matching
    // puppetable ownership belongs to the installed scene DemoDirector.
    void release_puppetable_demo_control(bool force_enable);

}  // namespace smgpc::compat
