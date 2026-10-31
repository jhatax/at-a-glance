# -*- mode: python; -*-

import os.path
import zipfile

top = '.'
out = 'build'


def options(ctx):
    ctx.load('pebble_sdk')


def configure(ctx):
    ctx.load('pebble_sdk')


def create_publish_bundle(task):
    with zipfile.ZipFile(task.inputs[0].abspath()) as source:
        with zipfile.ZipFile(task.outputs[0].abspath(), 'w') as target:
            target.comment = source.comment
            for entry in source.infolist():
                if entry.filename != 'pebble-js-app.js.map':
                    target.writestr(entry, source.read(entry))


def build(ctx):
    ctx.load('pebble_sdk')

    build_worker = os.path.exists('worker_src')
    binaries = []

    cached_env = ctx.env
    for platform in ctx.env.TARGET_PLATFORMS:
        ctx.env = ctx.all_envs[platform]
        # ctx.env.append_value('DEFINES', ['ATAGLANCE_DEBUG=1'])
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf = '{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        app_sources = ctx.path.ant_glob([
            'src/c/**/*.c',
            'src/modules/**/*.c',
        ])
        ctx.pbl_build(source=app_sources,
                      target=app_elf,
                      bin_type='app')

        if build_worker:
            worker_elf = '{}/pebble-worker.elf'.format(
                ctx.env.BUILD_DIR)
            binaries.append({
                'platform': platform,
                'app_elf': app_elf,
                'worker_elf': worker_elf,
            })
            worker_sources = ctx.path.ant_glob('worker_src/c/**/*.c')
            ctx.pbl_build(source=worker_sources,
                          target=worker_elf,
                          bin_type='worker')
        else:
            binaries.append({'platform': platform, 'app_elf': app_elf})
    ctx.env = cached_env

    ctx.set_group('bundle')
    ctx.pbl_bundle(
        binaries=binaries,
        js=ctx.path.ant_glob([
            'src/pkjs/**/*.js',
            'src/pkjs/**/*.json',
            'src/common/**/*.js',
            'node_modules/pebble-clay/**/*.js',
        ]),
        js_entry_file='src/pkjs/index.js',
    )

    bundle = ctx.path.get_bld().make_node(ctx.env.BUNDLE_NAME)
    ctx(rule=create_publish_bundle,
        source=bundle,
        target=bundle.change_ext('-publish.pbw'))
