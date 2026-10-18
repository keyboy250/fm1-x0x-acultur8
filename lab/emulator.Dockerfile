FROM emscripten/emsdk@sha256:77638e6c215a65f25a704c0c831d5c2463a8c0468a58c6cb82489dbe621dfa05
RUN apt-get update && apt-get install -y --no-install-recommends python3-pil build-essential && rm -rf /var/lib/apt/lists/*
ENV PYTHONPATH=/usr/lib/python3/dist-packages
