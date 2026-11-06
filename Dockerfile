FROM gcc:latest

# dependencies
RUN apt update && apt install cmake libspdlog-dev libfmt-dev -y

# create user
ENV APPUSER=mcmap
RUN groupadd -r $APPUSER && useradd --no-log-init -r -g $APPUSER $APPUSER
RUN mkdir /$APPUSER
RUN mkdir /input && chown $APPUSER:$APPUSER /input
RUN mkdir /output && chown $APPUSER:$APPUSER /output

# build
COPY . /$APPUSER
RUN cmake -S /$APPUSER -B /$APPUSER/build
RUN make -C /$APPUSER/build mcmap
RUN ln -s /$APPUSER/build/bin/* /bin/

# create a mount point for the minecraft save
VOLUME /input
# create a mount point to dump our image to
VOLUME /output

USER $APPUSER
ENTRYPOINT ["mcmap"]
CMD ["-file","/output/output.png","/input"]
