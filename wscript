#
# This file is the default set of rules to compile a Pebble application.
#
# Feel free to customize this to your needs.
#
import os.path

from waflib import Task

top = '.'
out = 'build'


def options(ctx):
    ctx.load('pebble_sdk')


def configure(ctx):
    """
    This method is used to configure your build. ctx.load(`pebble_sdk`) automatically configures
    a build for each valid platform in `targetPlatforms`. Platform-specific configuration: add your
    change after calling ctx.load('pebble_sdk') and make sure to set the correct environment first.
    Universal configuration: add your change prior to calling ctx.load('pebble_sdk').
    """
    ctx.load('pebble_sdk')

    # PBL_API_EXISTS() expands to defined(), and the SDK's linker script
    # makes a RWX segment; both only warn about the SDK itself
    for platform in ctx.env.TARGET_PLATFORMS:
        env = ctx.all_envs[platform]
        env.append_value('CFLAGS', ['-Wno-expansion-to-defined'])
        env.append_value('LINKFLAGS', ['-Wl,--no-warn-rwx-segments'])


def minify_js(ctx):
    """
    Makes the SDK's JS bundling also minify the bundle, see tools/minify.js.
    """
    merge_js = Task.classes['merge_js']
    if getattr(merge_js, 'minifies', False):
        return
    script = ctx.path.find_node('tools/minify.js').abspath()
    bundle = merge_js.run

    def run(task):
        result = bundle(task)
        if result:
            return result
        bld = task.generator.bld
        return bld.exec_command(['node', script, bld.env.NODE_PATH] +
                                [node.abspath() for node in task.outputs])

    merge_js.run = run
    merge_js.minifies = True


def build(ctx):
    ctx.load('pebble_sdk')
    minify_js(ctx)

    build_worker = os.path.exists('worker_src')
    binaries = []

    cached_env = ctx.env
    for platform in ctx.env.TARGET_PLATFORMS:
        ctx.env = ctx.all_envs[platform]
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf = '{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_build(source=ctx.path.ant_glob('src/c/**/*.c'), target=app_elf, bin_type='app')

        if build_worker:
            worker_elf = '{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': platform, 'app_elf': app_elf, 'worker_elf': worker_elf})
            ctx.pbl_build(source=ctx.path.ant_glob('worker_src/c/**/*.c'),
                          target=worker_elf,
                          bin_type='worker')
        else:
            binaries.append({'platform': platform, 'app_elf': app_elf})
    ctx.env = cached_env

    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries,
                   js=ctx.path.ant_glob(['src/pkjs/**/*.js',
                                         'src/pkjs/**/*.json',
                                         'src/common/**/*.js']),
                   js_entry_file='src/pkjs/index.js')
