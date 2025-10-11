FROM scratch
COPY ./yaths /
WORKDIR /
EXPOSE 8000
CMD ["/yaths", "--dir", "/data"]