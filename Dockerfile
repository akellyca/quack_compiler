# Build a Docker project atop the CIS 461/561 shared platform,
# which is an Ubuntu machine with gcc, build tools including Make,
# and the RE/flex lexical analyzer builder.
#
# To build:
#    docker build --tag=quack_compiler .
#
# To run in the resulting Docker container:
#   docker run -it quack_compiler
#
FROM michalyoung/cis461:base
COPY . /usr/src/quack_compiler
WORKDIR /usr/src/quack_compiler
RUN make
