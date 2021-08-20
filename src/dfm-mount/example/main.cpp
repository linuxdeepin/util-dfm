#include "include/dfmabstractdevice.h"
#include "include/dfmblockdevice.h"
#include <QVariantMap>

DFM_MOUNT_USE_NS
int main(int argc, char **argv) {

    DFMAbstractDevice dev(nullptr);
    dev.mount(QVariantMap());
    DFMBlockDevice dev2("path", nullptr);
    dev2.mount(QVariantMap());
    return 0;
}
