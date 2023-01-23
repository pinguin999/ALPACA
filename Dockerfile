FROM registry.fedoraproject.org/fedora-minimal:37
LABEL Name=ALPACA Version=0.0.1
RUN microdnf install -y ruby-devel libstdc++-static redhat-rpm-config git tar clang fontconfig-devel freetype-devel libvorbis-devel libepoxy-devel libwebp-devel boost-python3-devel python3-devel cmake ninja-build SDL2-devel openal-soft-devel clang-tools-extra python3-PyYAML libcxx-devel libasan mesa-dri-drivers xorg-x11-drv-dummy >/dev/null
RUN ruby --version
RUN gem install bundler
COPY ./Gemfile ./Gemfile
RUN bundle update
RUN bundle install --quiet
