from __future__ import print_function
import keras
import os
from tensorflow.keras.datasets import mnist
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Dropout, Flatten
from tensorflow.keras.layers import Conv2D, MaxPooling2D
from tensorflow.keras.preprocessing.image import ImageDataGenerator
from tensorflow.keras import backend as K
from tensorflow_estimator.python.estimator.export import export as export_helpers
from tensorflow.python.saved_model import builder as saved_model_builder
from tensorflow.python.saved_model import tag_constants, signature_constants
from tensorflow.python.saved_model.signature_def_utils_impl import predict_signature_def
import tensorflow as tf
import horovod.tensorflow.keras as hvd


# Horovod: initialize Horovod.
hvd.init()

# Horovod: pin GPU to be used to process local rank (one GPU per process)
config = tf.ConfigProto()
#config.gpu_options.allow_growth = True
#config.gpu_options.visible_device_list = str(hvd.local_rank())
K.set_session(tf.Session(config=config))

batch_size = 128
num_classes = 10

# Enough epochs to demonstrate learning rate warmup and the reduction of
# learning rate when training plateaues.
epochs = 24

# Input image dimensions
img_rows, img_cols = 28, 28

# The data, shuffled and split between train and test sets
(x_train, y_train), (x_test, y_test) = mnist.load_data()

# Determine how many batches are there in train and test sets
train_batches = len(x_train) // batch_size
test_batches = len(x_test) // batch_size

if K.image_data_format() == 'channels_first':
    x_train = x_train.reshape(x_train.shape[0], 1, img_rows, img_cols)
    x_test = x_test.reshape(x_test.shape[0], 1, img_rows, img_cols)
    input_shape = (1, img_rows, img_cols)
else:
    x_train = x_train.reshape(x_train.shape[0], img_rows, img_cols, 1)
    x_test = x_test.reshape(x_test.shape[0], img_rows, img_cols, 1)
    input_shape = (img_rows, img_cols, 1)

x_train = x_train.astype('float32')
x_test = x_test.astype('float32')
x_train /= 255
x_test /= 255
print('x_train shape:', x_train.shape)
print(x_train.shape[0], 'train samples')
print(x_test.shape[0], 'test samples')

# Convert class vectors to binary class matrices
y_train = tf.keras.utils.to_categorical(y_train, num_classes)
y_test = tf.keras.utils.to_categorical(y_test, num_classes)

model = Sequential()
model.add(Conv2D(32, kernel_size=(3, 3),
                 activation='relu',
                 input_shape=input_shape))
model.add(Conv2D(64, (3, 3), activation='relu'))
model.add(MaxPooling2D(pool_size=(2, 2)))
model.add(Dropout(0.25))
model.add(Flatten())
model.add(Dense(128, activation='relu'))
model.add(Dropout(0.5))
model.add(Dense(num_classes, activation='softmax'))

# Horovod: adjust learning rate based on number of GPUs.
opt = tf.keras.optimizers.Adadelta(lr=1.0 * hvd.size())

# Horovod: add Horovod Distributed Optimizer.
opt = hvd.DistributedOptimizer(opt)

model.compile(loss=tf.keras.losses.categorical_crossentropy,
              optimizer=opt,
              metrics=['accuracy'])

callbacks = [
    # Horovod: broadcast initial variable states from rank 0 to all other processes.
    # This is necessary to ensure consistent initialization of all workers when
    # training is started with random weights or restored from a checkpoint.
    hvd.callbacks.BroadcastGlobalVariablesCallback(0),

    # Horovod: average metrics among workers at the end of every epoch.
    #
    # Note: This callback must be in the list before the ReduceLROnPlateau,
    # TensorBoard or other metrics-based callbacks.
    hvd.callbacks.MetricAverageCallback(),

    # Horovod: using `lr = 1.0 * hvd.size()` from the very beginning leads to worse final
    # accuracy. Scale the learning rate `lr = 1.0` ---> `lr = 1.0 * hvd.size()` during
    # the first five epochs. See https://arxiv.org/abs/1706.02677 for details.
    hvd.callbacks.LearningRateWarmupCallback(warmup_epochs=5, verbose=1),

    # Reduce the learning rate if training plateaues.
    tf.keras.callbacks.ReduceLROnPlateau(patience=10, verbose=1),
]

# Horovod: save checkpoints only on worker 0 to prevent other workers from corrupting them.
if hvd.rank() == 0:
    callbacks.append(tf.keras.callbacks.ModelCheckpoint(
        './checkpoint-{epoch}.h5'))

# Set up ImageDataGenerators to do data augmentation for the training images.
train_gen = ImageDataGenerator(rotation_range=8, width_shift_range=0.08, shear_range=0.3,
                               height_shift_range=0.08, zoom_range=0.08)
test_gen = ImageDataGenerator()

# Train the model.
# Horovod: the training will randomly sample 1 / N batches of training data and
# 3 / N batches of validation data on every worker, where N is the number of workers.
# Over-sampling of validation data helps to increase probability that every validation
# example will be evaluated.
model.fit_generator(train_gen.flow(x_train, y_train, batch_size=batch_size),
                    steps_per_epoch=train_batches // hvd.size(),
                    callbacks=callbacks,
                    epochs=epochs,
                    verbose=1,
                    validation_data=test_gen.flow(
                        x_test, y_test, batch_size=batch_size),
                    validation_steps=3 * test_batches // hvd.size())

# Evaluate the model on the full data set.
score = model.evaluate(x_test, y_test, verbose=0)
print('Test loss:', score[0])
print('Test accuracy:', score[1])

# Save Model to Minio
if hvd.rank() == 0:
    print('Model Summary')
    model.summary()
    print('Exporting trained model to Minio Model Repo')
    base_path = os.environ['MODEL_BASE_PATH']

    # Option 1(Preferred) - Using Keras api and Tensorflow v1.13 version
    saved_model_path = tf.contrib.saved_model.save_keras_model(model, base_path)
    print('Model Saved to {} Using new Keras API!!!'.format(saved_model_path))
    # Option 2 - Tensorflow v1.13+ Builder saved_model api.
    # builder = saved_model_builder.SavedModelBuilder(base_path)

    # print(model.input)
    # print(model.outputs)

    # signature = predict_signature_def(inputs={"inputs": model.input},
    #                                   outputs={t.name:t for t in model.outputs})
    # print(signature)
    # K.set_learning_phase(0)
    # with K.get_session() as sess:
    #     builder.add_meta_graph_and_variables(sess=sess,
    #                                          tags=[tag_constants.SERVING],
    #                                          signature_def_map={'predict': signature})
    #     builder.save()
    # print('Model Saved to S3 Using Builder!!!')

    # Option 3 - Tensorflow v1.13 Will be deprecated in Tensorflow v2
    # tf.saved_model.simple_save(
    #     keras.backend.get_session(),
    #     export_path,
    #     inputs={'input_image': model.input},
    #     outputs={t.name: t for t in model.outputs})
