# Python image to use.
FROM python:3.8

# Set the working directory to /src/hdfs-writer
WORKDIR /src/inferenceApp

# Install librdkafka
RUN mkdir /librdkafka-dir && cd /librdkafka-dir
RUN git clone https://github.com/edenhill/librdkafka.git && \
cd librdkafka && \
./configure --prefix /usr && \
make && \
make install

#RUN export PYTHONPATH="/usr/bin/python3:/src/python-kafkaconsumer-inference-app/"

# copy the requirements file used for dependencies
COPY requirements.txt .

# Install any needed packages specified in requirements.txt
RUN pip install --trusted-host pypi.python.org -r requirements.txt

RUN pip install confluent-kafka
RUN pip install python-dateutil

# Install ptvsd for debugging
RUN pip install ptvsd



# Copy the rest of the working directory contents into the container at /app
COPY . ./

# Start the server when the container launches
CMD ["python3", "-m", "ptvsd", "--host", "localhost", "--port", "5000", "--wait", "/src/inferenceApp/main.py"]