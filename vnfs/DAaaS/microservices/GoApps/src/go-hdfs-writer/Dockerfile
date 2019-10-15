# Use base golang image from Docker Hub
FROM golang:1.12.7

# Download the dlv (delve) debugger for go (you can comment this out if unused)
RUN go get -u github.com/go-delve/delve/cmd/dlv

WORKDIR /src/hdfs-writer

RUN mkdir /librdkafka-dir && cd /librdkafka-dir
RUN git clone https://github.com/edenhill/librdkafka.git && \ 
cd librdkafka && \
./configure --prefix /usr && \
make && \
make install

# Install dependencies in go.mod and go.sum
COPY go.mod go.sum ./
RUN go mod download

# Copy rest of the application source code
COPY . ./

# Compile the application to /app.
RUN go build -o /hdfs-writer -v ./cmd/hdfs-writer

# If you want to use the debugger, you need to modify the entrypoint to the
# container and point it to the "dlv debug" command:
#   * UNCOMMENT the following ENTRYPOINT statement,
#   * COMMENT OUT the last ENTRYPOINT statement
# Start the "dlv debug" server on port 3000 of the container. (Note that the
# application process will NOT start until the debugger is attached.)
#ENTRYPOINT ["dlv", "debug", "./cmd/hdfs-writer",  "--api-version=2", "--headless", "--listen=:3001", "--log", "--log-dest=/home.dlv.log"]

# If you want to run WITHOUT the debugging server:
#   * COMMENT OUT the previous ENTRYPOINT statements,
#   * UNCOMMENT the following ENTRYPOINT statement.
#ENTRYPOINT ["/bin/sleep", "3600"]
ENTRYPOINT ["/hdfs-writer"]