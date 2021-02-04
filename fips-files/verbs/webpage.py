"""fips verb to build the examples webpage"""

import os
import datetime
import yaml
import shutil
import subprocess
import glob
from string import Template

from mod import log, util, project, emscripten, android

# items attributes
items = [
    [ 'Getting started', [
        [ 'Hello Window', 'https://learnopengl.com/Getting-started/Hello-Window', '1-3-hello-window', [
            [ 'rendering', '1-3-1-rendering', '1-rendering.c', None],
        ]],
        [ 'Hello Triangle', 'https://learnopengl.com/Getting-started/Hello-Triangle', '1-4-hello-triangle', [
            [ 'triangle', '1-4-1-triangle', '1-triangle.c', '1-triangle.glsl'],
            [ 'quad', '1-4-2-quad', '2-quad.c', '2-quad.glsl'],
            [ 'quad-wireframe', '1-4-3-quad-wireframe', '3-quad-wireframe.c', '3-quad-wireframe.glsl']
        ]],
        [ 'Shaders', 'https://learnopengl.com/Getting-started/Shaders', '1-5-shaders', [
            [ 'in-out', '1-5-1-in-out', '1-in-out.c', '1-in-out.glsl'],
            [ 'uniforms', '1-5-2-uniforms', '2-uniforms.c', '2-uniforms.glsl'],
            [ 'attributes', '1-5-3-attributes', '3-attributes.c', '3-attributes.glsl']
        ]],
        [ 'Textures', 'https://learnopengl.com/Getting-started/Textures', '1-6-textures', [
            [ 'texture', '1-6-1-texture', '1-texture.c', '1-texture.glsl'],
            [ 'texture-blend', '1-6-2-texture-blend', '2-texture-blend.c', '2-texture-blend.glsl'],
            [ 'multiple-textures', '1-6-3-multiple-textures', '3-multiple-textures.c', '3-multiple-textures.glsl']
        ]],
        [ 'Transformations', 'https://learnopengl.com/Getting-started/Transformations', '1-7-transformations', [
            [ 'scale-rotate', '1-7-1-scale-rotate', '1-scale-rotate.c', 'transformations.glsl'],
            [ 'rotate-translate', '1-7-2-rotate-translate', '2-rotate-translate.c', 'transformations.glsl'],
        ]],
        [ 'Coordinate Systems', 'https://learnopengl.com/Getting-started/Coordinate-Systems', '1-8-coordinate-systems', [
            [ 'plane', '1-8-1-plane', '1-plane.c', 'shaders.glsl'],
            [ 'cube', '1-8-2-cube', '2-cube.c', 'shaders.glsl'],
            [ 'more-cubes', '1-8-3-more-cubes', '3-more-cubes.c', 'shaders.glsl']
        ]],
        [ 'Camera', 'https://learnopengl.com/Getting-started/Camera', '1-9-camera', [
            [ 'lookat', '1-9-1-lookat', '1-lookat.c', 'shaders.glsl'],
            [ 'walk', '1-9-2-walk', '2-walk.c', 'shaders.glsl'],
            [ 'look', '1-9-3-look', '3-look.c', 'shaders.glsl']
        ]]
    ]],
    [ 'Lighting', [
        [ 'Colors', 'https://learnopengl.com/Lighting/Colors', '2-1-colors', [
            [ 'scene', '2-1-1-scene', '1-scene.c', 'shaders.glsl']
        ]],
        [ 'Basic Lighting', 'https://learnopengl.com/Lighting/Basic-Lighting', '2-2-basic-lighting', [
            [ 'ambient', '2-2-1-ambient', '1-ambient.c', '1-ambient.glsl'],
            [ 'diffuse', '2-2-2-diffuse', '2-diffuse.c', '2-diffuse.glsl'],
            [ 'specular', '2-2-3-specular', '3-specular.c', '3-specular.glsl']
        ]],
        [ 'Materials', 'https://learnopengl.com/Lighting/Materials', '2-3-materials', [
            [ 'material', '2-3-1-material', '1-material.c', '1-material.glsl'],
            [ 'light', '2-3-2-light', '2-light.c', '2-light.glsl'],
            [ 'light-colors', '2-3-3-light-colors', '3-light-colors.c', '3-light-colors.glsl']
        ]],
        [ 'Lighting Maps', 'https://learnopengl.com/Lighting/Lighting-maps', '2-4-lighting-maps', [
            [ 'diffuse-map', '2-4-1-diffuse-map', '1-diffuse-map.c', '1-diffuse-map.glsl'],
            [ 'specular-map', '2-4-2-specular-map', '2-specular-map.c', '2-specular-map.glsl'],
        ]],
        [ 'Light Casters', 'https://learnopengl.com/Lighting/Light-casters', '2-5-light-casters', [
            [ 'directional-light', '2-5-1-directional-light', '1-directional-light.c', '1-directional-light.glsl'],
            [ 'point-light', '2-5-2-point-light', '2-point-light.c', '2-point-light.glsl'],
            [ 'spot-light', '2-5-3-spot-light', '3-spot-light.c', '3-spot-light.glsl'],
            [ 'soft-spot-light', '2-5-4-soft-spot-light', '4-soft-spot-light.c', '4-soft-spot-light.glsl'],
        ]],
        [ 'Multiple Lights', 'https://learnopengl.com/Lighting/Multiple-lights', '2-6-multiple-lights', [
            [ 'combined-lights', '2-6-1-combined-lights', '1-combined-lights.c', '1-combined-lights.glsl'],
        ]]
    ]],
    [ 'Model Loading', [
        [ 'Model', 'https://learnopengl.com/Model-Loading/Model', '3-1-model', [
            [ 'backpack-diffuse', '3-1-1-backpack-diffuse', '1-backpack-diffuse.c', '1-backpack-diffuse.glsl'],
            [ 'backpack-lights', '3-1-2-backpack-lights', '2-backpack-lights.c', '2-backpack-lights.glsl']
        ]]
    ]],
    [ 'Advanced OpenGL', [
        [ 'Depth Testing', 'https://learnopengl.com/Advanced-OpenGL/Depth-testing', '4-1-depth-testing', [
            [ 'depth-always', '4-1-1-depth-always', '1-depth-always.c', '1-depth-always.glsl'],
            [ 'depth-less', '4-1-2-depth-less', '2-depth-less.c', '2-depth-less.glsl'],
            [ 'depth-buffer', '4-1-3-depth-buffer', '3-depth-buffer.c', '3-depth-buffer.glsl'],
            [ 'linear-depth-buffer', '4-1-4-linear-depth-buffer', '4-linear-depth-buffer.c', '4-linear-depth-buffer.glsl']
        ]],
        [ 'Stencil Testing', 'https://learnopengl.com/Advanced-OpenGL/Stencil-testing', '4-2-stencil-testing', [
            [ 'object-outlining', '4-2-1-object-outlining', '1-object-outlining.c', '1-object-outlining.glsl'],
        ]],
        [ 'Blending', 'https://learnopengl.com/Advanced-OpenGL/Blending', '4-3-blending', [
            [ 'grass-opaque', '4-3-1-grass-opaque', '1-grass-opaque.c', '1-grass-opaque.glsl'],
            [ 'grass-transparent', '4-3-2-grass-transparent', '2-grass-transparent.c', '2-grass-transparent.glsl'],
            [ 'blending', '4-3-3-blending', '3-blending.c', '3-blending.glsl'],
            [ 'blending-sorted', '4-3-4-blending-sorted', '4-blending-sorted.c', '4-blending-sorted.glsl']
        ]],
        [ 'Face Culling', 'https://learnopengl.com/Advanced-OpenGL/Face-culling', '4-4-face-culling', [
            [ 'cull-front', '4-4-1-cull-front', '1-cull-front.c', '1-cull-front.glsl']
        ]],
        [ 'Framebuffers', 'https://learnopengl.com/Advanced-OpenGL/Framebuffers', '4-5-framebuffers', [
            [ 'render-to-texture', '4-5-1-render-to-texture', '1-render-to-texture.c', '1-render-to-texture.glsl'],
            [ 'inversion', '4-5-2-inversion', '2-inversion.c', '2-inversion.glsl'],
            [ 'grayscale', '4-5-3-grayscale', '3-grayscale.c', '3-grayscale.glsl'],
            [ 'sharpen', '4-5-4-sharpen', '4-sharpen.c', '4-sharpen.glsl'],
            [ 'blur', '4-5-5-blur', '5-blur.c', '5-blur.glsl'],
            [ 'edge-detection', '4-5-6-edge-detection', '6-edge-detection.c', '6-edge-detection.glsl']
        ]],
        [ 'Cubemaps', 'https://learnopengl.com/Advanced-OpenGL/Cubemaps', '4-6-cubemaps', [
            [ 'skybox', '4-6-1-skybox', '1-skybox.c', '1-skybox.glsl'],
            [ 'relfection-cube', '4-6-2-reflection-cube', '2-reflection-cube.c', '2-reflection-cube.glsl'],
            [ 'relfection-backpack', '4-6-3-reflection-backpack', '3-reflection-backpack.c', '3-reflection-backpack.glsl'],
            [ 'refraction-cube', '4-6-4-refraction-cube', '4-refraction-cube.c', '4-refraction-cube.glsl'],
            [ 'refraction-backpack', '4-6-5-refraction-backpack', '5-refraction-backpack.c', '5-refraction-backpack.glsl'],
        ]],
        [ 'Advanced GLSL', 'https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL', '4-8-advanced-glsl', [
            [ 'point-size', '4-8-1-point-size', '1-point-size.c', '1-point-size.glsl'],
            [ 'frag-coord', '4-8-2-frag-coord', '2-frag-coord.c', '2-frag-coord.glsl'],
            [ 'front-facing', '4-8-3-front-facing', '3-front-facing.c', '3-front-facing.glsl'],
            [ 'uniform-buffers', '4-8-4-uniform-buffers', '4-uniform-buffers.c', '4-uniform-buffers.glsl'],
        ]],
        [ 'Geometry Shader', 'https://learnopengl.com/Advanced-OpenGL/Geometry-Shader', '4-9-geometry-shader', [
            [ 'lines', '4-9-1-lines', '1-lines.c', '1-lines.glsl'],
            [ 'houses', '4-9-2-houses', '2-houses.c', '2-houses.glsl'],
            [ 'exploding-object', '4-9-3-exploding-object', '3-exploding-object.c', '3-exploding-object.glsl'],
            [ 'visualizing-normals', '4-9-4-visualizing-normals', '4-visualizing-normals.c', '4-visualizing-normals.glsl'],
        ]],
        [ 'Instancing', 'https://learnopengl.com/Advanced-OpenGL/Instancing', '4-10-instancing', [
            [ 'instancing', '4-10-1-instancing', '1-instancing.c', '1-instancing.glsl'],
            [ 'instanced-arrays', '4-10-2-instanced-arrays', '2-instanced-arrays.c', '2-instanced-arrays.glsl'],
            [ 'asteroid-field', '4-10-3-asteroid-field', '3-asteroid-field.c', '3-asteroid-field.glsl'],
            [ 'asteroid-field-instanced', '4-10-4-asteroid-field-instanced', '4-asteroid-field-instanced.c', '4-asteroid-field-instanced.glsl'],
        ]],
        [ 'Anti Aliasing', 'https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing', '4-11-anti-aliasing', [
            [ 'msaa', '4-11-1-msaa', '1-msaa.c', '1-msaa.glsl'],
            [ 'offscreen-msaa', '4-11-2-offscreen-msaa', '2-offscreen-msaa.c', '2-offscreen-msaa.glsl'],
            [ 'grayscale-msaa', '4-11-3-grayscale-msaa', '3-grayscale-msaa.c', '3-grayscale-msaa.glsl'],
        ]],
        [ 'Advanced Lighting', 'https://learnopengl.com/Advanced-Lighting/Advanced-Lighting', '5-1-advanced-lighting', [
            [ 'blinn-phong', '5-1-1-blinn-phong', '1-blinn-phong.c', '1-blinn-phong.glsl'],
        ]],
        [ 'Gamma Correction', 'https://learnopengl.com/Advanced-Lighting/Gamma-Correction', '5-2-gamma-correction', [
            [ 'gamma-correction', '5-2-1-gamma-correction', '1-gamma-correction.c', '1-gamma-correction.glsl'],
        ]],
        [ 'Shadow Mapping', 'https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping', '5-3-shadow-mapping', [
            [ 'mapping-depth', '5-3-1-mapping-depth', '1-mapping-depth.c', '1-mapping-depth.glsl'],
            [ 'rendering-shadows', '5-3-2-rendering-shadows', '2-rendering-shadows.c', '2-rendering-shadows.glsl'],
            [ 'improved-shadows', '5-3-3-improved-shadows', '3-improved-shadows.c', '3-improved-shadows.glsl'],
        ]],
        [ 'Point Shadows', 'https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows', '5-4-point-shadows', [
            [ 'omnidirectional-depth', '5-4-1-omnidirectional-depth', '1-omnidirectional-depth.c', '1-omnidirectional-depth.glsl'],
            [ 'omnidir-shadows', '5-4-2-omnidirectional-shadows', '2-omnidirectional-shadows.c', '2-omnidirectional-shadows.glsl'],
            [ 'omnidirectional-PCF', '5-4-3-omnidirectional-PCF', '3-omnidirectional-PCF.c', '3-omnidirectional-PCF.glsl'],
        ]],
        [ 'Normal Mapping', 'https://learnopengl.com/Advanced-Lighting/Normal-Mapping', '5-5-normal-mapping', [
            [ 'normal-mapping', '5-5-1-normal-mapping', '1-normal-mapping.c', '1-normal-mapping.glsl'],
            [ 'tangent-space', '5-5-2-tangent-space', '2-tangent-space.c', '2-tangent-space.glsl'],
            [ 'complex-object', '5-5-3-complex-object', '3-complex-object.c', '3-complex-object.glsl'],
        ]]
    ]]
]

# assets that must also be copied
assets = [
    "awesomeface.png",
    "backpack.mtl",
    "backpack.obj",
    "backpack_diffuse.jpg",
    "backpack_normal.png",
    "backpack_specular.jpg",
    "brickwall.jpg",
    "brickwall_normal.jpg",
    "container.jpg",
    "container2.png",
    "container2_specular.png",
    "grass.png",
    "marble.jpg",
    "mars.png",
    "metal.png",
    "planet.mtl",
    "planet.obj",
    "rock.mtl",
    "rock.obj",
    "rock.png",
    "skybox_right.jpg",
    "skybox_left.jpg",
    "skybox_top.jpg",
    "skybox_bottom.jpg",
    "skybox_front.jpg",
    "skybox_back.jpg",
    "transparent_window.png",
    "uv_grid.png",
    "wood.png"
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
    html = templ.safe_substitute(samples=content, date=datetime.date.today().strftime("%B %d %Y"))
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

