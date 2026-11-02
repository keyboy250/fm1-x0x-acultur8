# Target-only standard header paths repair the preserved Windows toolchain layout.
import importlib.util,sys
from pathlib import Path
root=Path.cwd()
spec=importlib.util.spec_from_file_location('private_target_build',root/'tools/build.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
module.CFLAGS+=['-resource-dir','/toolchain/common/lib/clang/4.0.1','-isystem','/toolchain/pi32v2/include']
sys.argv=['tools/build.py','--release','0.1.60-stoppedtrace','--sdk','/sdk']
module.main()
