"""fips verb to build the examples webpage"""

import os
import yaml
import shutil
import subprocess
import glob
from string import Template

from mod import log, util, project, emscripten, android

# items attributes
items = [
    [ 'Getting started', [
        [ 'Hello Window', 'https://learnopengl.com/Getting-started/Hello-Window', '2-3-hello-window', [
            [ 'rendering', '2-3-1-rendering', '1-rendering.c', None],
        ]],
        [ 'Hello Triangle', 'https://learnopengl.com/Getting-started/Hello-Triangle', '2-4-hello-triangle', [
            [ 'triangle', '2-4-1-triangle', '1-triangle.c', '1-triangle.glsl'],
            [ 'quad', '2-4-2-quad', '2-quad.c', '1-quad.glsl'],
            [ 'quad-wireframe', '2-4-3-quad-wireframe', '3-quad-wireframe.c', '3-quad-wireframe.glsl']
        ]],
        [ 'Shaders', 'https://learnopengl.com/Getting-started/Shaders', '2-5-shaders', [
            [ 'in-out', '2-5-1-in-out', '1-in-out.c', '1-in-out.glsl'],
            [ 'uniforms', '2-5-2-uniforms', '2-uniforms.c', '2-uniforms.glsl'],
            [ 'attributes', '2-5-3-attributes', '3-attributes.c', '3-attributes.glsl']
        ]],
        [ 'Textures', 'https://learnopengl.com/Getting-started/Textures', '2-6-textures', [
            [ 'texture', '2-6-1-texture', '1-texture.c', '1-texture.glsl'],
            [ 'texture-blend', '2-6-2-texture-blend', '2-texture-blend.c', '2-texture-blend.glsl'],
            [ 'multiple-textures', '2-6-3-multiple-textures', '3-multiple-textures.c', '3-multiple-textures.glsl']
        ]],
        [ 'Transformations', 'https://learnopengl.com/Getting-started/Transformations', '2-7-transformations', [
            [ 'scale-rotate', '2-7-1-scale-rotate', '1-scale-rotate.c', 'transformations.glsl'],
            [ 'rotate-translate', '2-7-2-rotate-translate', '2-rotate-translate.c', 'transformations.glsl'],
        ]],
        [ 'Coordinate Systems', 'https://learnopengl.com/Getting-started/Coordinate-Systems', '2-8-coordinate-systems', [
            [ 'plane', '2-8-1-plane', '1-plane.c', 'shaders.glsl'],
            [ 'cube', '2-8-2-cube', '2-cube.c', 'shaders.glsl'],
            [ 'more-cubes', '2-8-3-more-cubes', '3-more-cubes.c', 'shaders.glsl']
        ]],
        [ 'Camera', 'https://learnopengl.com/Getting-started/Camera', '2-9-camera', [
            [ 'lookat', '2-9-1-lookat', '1-lookat.c', 'shaders.glsl'],
            [ 'walk', '2-9-2-walk', '2-walk.c', 'shaders.glsl'],
            [ 'look', '2-9-3-look', '3-look.c', 'shaders.glsl']
        ]]
    ]],
    [ 'Lighting', [
        [ 'Colors', 'https://learnopengl.com/Lighting/Colors', '3-1-colors', [
            [ 'scene', '3-1-1-scene', '1-scene.c', 'shaders.glsl']
        ]],
        [ 'Basic Lighting', 'https://learnopengl.com/Lighting/Basic-Lighting', '3-2-basic-lighting', [
            [ 'ambient', '3-2-1-ambient', '1-ambient.c', 'ambient.glsl'],
            [ 'diffuse', '3-2-2-diffuse', '2-diffuse.c', 'diffuse.glsl'],
            [ 'specular', '3-2-3-specular', '3-specular.c', 'specular.glsl']
        ]]
    ]]
]

# assets that must also be copied
assets = [
    "awesomeface.png",
    "container.jpg"
]

# webpage template arguments
GitHubExamplesURL = 'https://github.com/geertarien/learnopengl-examples/tree/master/src'

# build configuration
BuildConfig = 'webgl2-wasm-ninja-release'

# targets deploy directory
WebpageDeployDir = 'learnopengl-examples-webpage'

#-------------------------------------------------------------------------------
def deploy_webpage(fips_dir, proj_dir, webpage_dir) :
    """builds the final webpage under under fips-deploy/learnopengl-examples"""
    ws_dir = util.get_workspace_dir(fips_dir)
    wasm_deploy_dir = '{}/fips-deploy/learnopengl-examples/{}'.format(ws_dir, BuildConfig)

    # create directories
    if not os.path.exists(webpage_dir):
        os.makedirs(webpage_dir)

    # build the thumbnail gallery
    content = ''
    for chapter in items:
        chapter_title = chapter[0]
        lessons = chapter[1]
        content += '<h2>{}</i></h2>\n'.format(chapter_title)
        for lesson in lessons:
            lesson_title = lesson[0]
            lesson_link = lesson[1]
            examples = lesson[3]
            content += '<article>\n'
            content += '<section class="header"><h3><a href="{}">{} <i class="icon-link-ext"></i></a></h3></section>\n'.format(lesson_link, lesson_title)
            content += '<section class="group examples">\n'
            for example in examples:
                name = example[0]
                filename = example[1]
                log.info('> adding thumbnail for {}'.format(filename))
                url = "{}.html".format(filename)
                img_name = filename + '.jpg'
                img_path = proj_dir + '/webpage/' + img_name
                if not os.path.exists(img_path):
                    img_name = 'dummy.jpg'
                    img_path = proj_dir + 'webpage/dummy.jpg'
                content += '<figure class="col-15">\n'
                content += '<figcaption><h4>{}</h4></figcaption>\n'.format(name)
                content += '<div><img class="responsive" src="{}" alt=""></div>\n'.format(img_name)
                content += '<a href="{}">Read More</a>\n'.format( url)
                content += '</figure>\n'
            content += '</section>\n'
            content += '</article>\n'
        content += '<hr>\n'

    # populate the html template, and write to the build directory
    with open(proj_dir + '/webpage/index.html', 'r') as f:
        templ = Template(f.read())
    html = templ.safe_substitute(samples=content)
    with open(webpage_dir + '/index.html', 'w') as f :
        f.write(html)

    # copy other required files
    for name in ['dummy.jpg', 'favicon.png', 'fontello.woff', 'fontello.woff2']:
        log.info('> copy file: {}'.format(name))
        shutil.copy(proj_dir + '/webpage/' + name, webpage_dir + '/' + name)

    # generate WebAssembly HTML pages
    if emscripten.check_exists(fips_dir):
        for chapter in items:
            lessons = chapter[1]
            for lesson in lessons:
                dir = lesson[2]
                examples = lesson[3]
                for example in examples:
                    filename = example[1]
                    source = example[2]
                    glsl = example[3]
                    log.info('> generate wasm HTML page: {}'.format(filename))
                    for ext in ['wasm', 'js'] :
                        src_path = '{}/{}.{}'.format(wasm_deploy_dir, filename, ext)
                        if os.path.isfile(src_path) :
                            shutil.copy(src_path, '{}/'.format(webpage_dir))
                        with open(proj_dir + '/webpage/wasm.html', 'r') as f :
                            templ = Template(f.read())
                        src_url = '{}/{}/{}'.format(GitHubExamplesURL, dir, source)
                        if glsl is None:
                            glsl_url = "."
                            glsl_hidden = "hidden"
                        else:
                            glsl_url = '{}/{}/{}'.format(GitHubExamplesURL, dir, glsl)
                            glsl_hidden = ""
                        html = templ.safe_substitute(name=filename, prog=filename, source=src_url, glsl=glsl_url, hidden=glsl_hidden)
                        with open('{}/{}.html'.format(webpage_dir, filename), 'w') as f :
                            f.write(html)

    # copy assets from deploy directory
    for asset in assets:
        log.info('> copy asset file: {}'.format(asset))
        src_path = '{}/{}'.format(wasm_deploy_dir, asset)
        if os.path.isfile(src_path):
            shutil.copy(src_path, webpage_dir)

    # copy the screenshots
    for chapter in items:
        lessons = chapter[1]
        for lesson in lessons:
            examples = lesson[3]
            for example in examples:
                img_name = example[1] + '.jpg'
                img_path = proj_dir + '/webpage/' + img_name
                if os.path.exists(img_path):
                    log.info('> copy screenshot: {}'.format(img_name))
                    shutil.copy(img_path, webpage_dir + '/' + img_name)

#-------------------------------------------------------------------------------
def build_deploy_webpage(fips_dir, proj_dir, rebuild) :
    # if webpage dir exists, clear it first
    ws_dir = util.get_workspace_dir(fips_dir)
    webpage_dir = '{}/fips-deploy/{}'.format(ws_dir, WebpageDeployDir)
    if rebuild :
        if os.path.isdir(webpage_dir) :
            shutil.rmtree(webpage_dir)
    if not os.path.isdir(webpage_dir) :
        os.makedirs(webpage_dir)

    # compile examples
    if emscripten.check_exists(fips_dir) :
        project.gen(fips_dir, proj_dir, BuildConfig)
        project.build(fips_dir, proj_dir, BuildConfig)

    # deploy the webpage
    deploy_webpage(fips_dir, proj_dir, webpage_dir)

    log.colored(log.GREEN, 'Generated Examples web page under {}.'.format(webpage_dir))

#-------------------------------------------------------------------------------
def serve_webpage(fips_dir, proj_dir) :
    ws_dir = util.get_workspace_dir(fips_dir)
    webpage_dir = '{}/fips-deploy/{}'.format(ws_dir, WebpageDeployDir)
    p = util.get_host_platform()
    if p == 'osx' :
        try :
            subprocess.call(
                'http-server -c-1 -g -o'.format(fips_dir),
                cwd = webpage_dir, shell=True)
        except KeyboardInterrupt :
            pass
    elif p == 'win':
        try:
            subprocess.call(
                'http-server -c-1 -g -o'.format(fips_dir),
                cwd = webpage_dir, shell=True)
        except KeyboardInterrupt:
            pass
    elif p == 'linux':
        try:
            subprocess.call(
                'http-server -c-1 -g -o'.format(fips_dir),
                cwd = webpage_dir, shell=True)
        except KeyboardInterrupt:
            pass

#-------------------------------------------------------------------------------
def run(fips_dir, proj_dir, args) :
    if len(args) > 0 :
        if args[0] == 'build' :
            build_deploy_webpage(fips_dir, proj_dir, False)
        elif args[0] == 'rebuild' :
            build_deploy_webpage(fips_dir, proj_dir, True)
        elif args[0] == 'serve' :
            serve_webpage(fips_dir, proj_dir)
        else :
            log.error("Invalid param '{}', expected 'build' or 'serve'".format(args[0]))
    else :
        log.error("Param 'build' or 'serve' expected")

#-------------------------------------------------------------------------------
def help() :
    log.info(log.YELLOW +
             'fips webpage build\n' +
             'fips webpage rebuild\n' +
             'fips webpage serve\n' +
             log.DEF +
             '    build learnopengl examples webpage')

